/*
 * Implementation of a TPM driver for the TPM TIS interface
 *
 * Copyright (C) 2006-2013 IBM Corporation
 * Copyright (C) 2015 Intel Corporation
 *
 * Authors:
 *     Stefan Berger <stefanb@linux.vnet.ibm.com>
 *     Quan Xu <quan.xu@intel.com>
 *
 * This file may be distributed under the terms of the GNU
 * LGPLv3 license.
 */

#include "config.h"
#include "util.h"
#include "tpm.h"

static u32 tis_default_timeouts[4] = {
    TIS_DEFAULT_TIMEOUT_A,
    TIS_DEFAULT_TIMEOUT_B,
    TIS_DEFAULT_TIMEOUT_C,
    TIS_DEFAULT_TIMEOUT_D,
};

static u32 tpm_default_durations[3] = {
    TPM_DEFAULT_DURATION_SHORT,
    TPM_DEFAULT_DURATION_MEDIUM,
    TPM_DEFAULT_DURATION_LONG,
};


/* if device is not there, return '0', '1' otherwise */
static u32 tis_probe(void)
{
    u32 rc = 0;
    u32 didvid = readl(TIS_REG(0, TIS_REG_DID_VID));

    if ((didvid != 0) && (didvid != 0xffffffff))
        rc = 1;

    return rc;
}

static u32 tis_init(void)
{
    writeb(TIS_REG(0, TIS_REG_INT_ENABLE), 0);

    if (tpm_drivers[TIS_DRIVER_IDX].durations == NULL) {
        u32 *durations = malloc_low(sizeof(tpm_default_durations));
        if (durations)
            memcpy(durations, tpm_default_durations,
                   sizeof(tpm_default_durations));
        else
            durations = tpm_default_durations;
        tpm_drivers[TIS_DRIVER_IDX].durations = durations;
    }

    if (tpm_drivers[TIS_DRIVER_IDX].timeouts == NULL) {
        u32 *timeouts = malloc_low(sizeof(tis_default_timeouts));
        if (timeouts)
            memcpy(timeouts, tis_default_timeouts,
                   sizeof(tis_default_timeouts));
        else
            timeouts = tis_default_timeouts;
        tpm_drivers[TIS_DRIVER_IDX].timeouts = timeouts;
    }

    return 1;
}


static void set_timeouts(u32 timeouts[4], u32 durations[3])
{
    u32 *tos = tpm_drivers[TIS_DRIVER_IDX].timeouts;
    u32 *dus = tpm_drivers[TIS_DRIVER_IDX].durations;

    if (tos && tos != tis_default_timeouts && timeouts)
        memcpy(tos, timeouts, 4 * sizeof(u32));
    if (dus && dus != tpm_default_durations && durations)
        memcpy(dus, durations, 3 * sizeof(u32));
}


static u32 tis_wait_sts(u8 locty, u32 time, u8 mask, u8 expect)
{
    u32 rc = 1;

    while (time > 0) {
        u8 sts = readb(TIS_REG(locty, TIS_REG_STS));
        if ((sts & mask) == expect) {
            rc = 0;
            break;
        }
        msleep(1);
        time--;
    }
    return rc;
}

static u32 tis_activate(u8 locty)
{
    u32 rc = 0;
    u8 acc;
    int l;
    u32 timeout_a = tpm_drivers[TIS_DRIVER_IDX].timeouts[TIS_TIMEOUT_TYPE_A];

    if (!(readb(TIS_REG(locty, TIS_REG_ACCESS)) &
          TIS_ACCESS_ACTIVE_LOCALITY)) {
        /* release locality in use top-downwards */
        for (l = 4; l >= 0; l--)
            writeb(TIS_REG(l, TIS_REG_ACCESS),
                   TIS_ACCESS_ACTIVE_LOCALITY);
    }

    /* request access to locality */
    writeb(TIS_REG(locty, TIS_REG_ACCESS), TIS_ACCESS_REQUEST_USE);

    acc = readb(TIS_REG(locty, TIS_REG_ACCESS));
    if ((acc & TIS_ACCESS_ACTIVE_LOCALITY)) {
        writeb(TIS_REG(locty, TIS_REG_STS), TIS_STS_COMMAND_READY);
        rc = tis_wait_sts(locty, timeout_a,
                          TIS_STS_COMMAND_READY, TIS_STS_COMMAND_READY);
    }

    return rc;
}

static u32 tis_find_active_locality(void)
{
    u8 locty;

    for (locty = 0; locty <= 4; locty++) {
        if ((readb(TIS_REG(locty, TIS_REG_ACCESS)) &
             TIS_ACCESS_ACTIVE_LOCALITY))
            return locty;
    }

    tis_activate(0);

    return 0;
}

static u32 tis_ready(void)
{
    u32 rc = 0;
    u8 locty = tis_find_active_locality();
    u32 timeout_b = tpm_drivers[TIS_DRIVER_IDX].timeouts[TIS_TIMEOUT_TYPE_B];

    writeb(TIS_REG(locty, TIS_REG_STS), TIS_STS_COMMAND_READY);
    rc = tis_wait_sts(locty, timeout_b,
                      TIS_STS_COMMAND_READY, TIS_STS_COMMAND_READY);

    return rc;
}

static u32 tis_senddata(const u8 *const data, u32 len)
{
    u32 rc = 0;
    u32 offset = 0;
    u32 end = 0;
    u16 burst = 0;
    u32 ctr = 0;
    u8 locty = tis_find_active_locality();
    u32 timeout_d = tpm_drivers[TIS_DRIVER_IDX].timeouts[TIS_TIMEOUT_TYPE_D];

    do {
        while (burst == 0 && ctr < timeout_d) {
               burst = readl(TIS_REG(locty, TIS_REG_STS)) >> 8;
            if (burst == 0) {
                msleep(1);
                ctr++;
            }
        }

        if (burst == 0) {
            rc = TCG_RESPONSE_TIMEOUT;
            break;
        }

        while (1) {
            writeb(TIS_REG(locty, TIS_REG_DATA_FIFO), data[offset++]);
            burst--;

            if (burst == 0 || offset == len)
                break;
        }

        if (offset == len)
            end = 1;
    } while (end == 0);

    return rc;
}

static u32 tis_readresp(u8 *buffer, u32 *len)
{
    u32 rc = 0;
    u32 offset = 0;
    u32 sts;
    u8 locty = tis_find_active_locality();

    while (offset < *len) {
        buffer[offset] = readb(TIS_REG(locty, TIS_REG_DATA_FIFO));
        offset++;
        sts = readb(TIS_REG(locty, TIS_REG_STS));
        /* data left ? */
        if ((sts & TIS_STS_DATA_AVAILABLE) == 0)
            break;
    }

    *len = offset;

    return rc;
}


static u32 tis_waitdatavalid(void)
{
    u32 rc = 0;
    u8 locty = tis_find_active_locality();
    u32 timeout_c = tpm_drivers[TIS_DRIVER_IDX].timeouts[TIS_TIMEOUT_TYPE_C];

    if (tis_wait_sts(locty, timeout_c, TIS_STS_VALID, TIS_STS_VALID) != 0)
        rc = TCG_NO_RESPONSE;

    return rc;
}

static u32 tis_waitrespready(enum tpmDurationType to_t)
{
    u32 rc = 0;
    u8 locty = tis_find_active_locality();
    u32 timeout = tpm_drivers[TIS_DRIVER_IDX].durations[to_t];

    writeb(TIS_REG(locty ,TIS_REG_STS), TIS_STS_TPM_GO);

    if (tis_wait_sts(locty, timeout,
                     TIS_STS_DATA_AVAILABLE, TIS_STS_DATA_AVAILABLE) != 0)
        rc = TCG_NO_RESPONSE;

    return rc;
}


struct tpm_driver tpm_drivers[TPM_NUM_DRIVERS] = {
    [TIS_DRIVER_IDX] =
        {
            .timeouts      = NULL,
            .durations     = NULL,
            .set_timeouts  = set_timeouts,
            .probe         = tis_probe,
            .init          = tis_init,
            .activate      = tis_activate,
            .ready         = tis_ready,
            .senddata      = tis_senddata,
            .readresp      = tis_readresp,
            .waitdatavalid = tis_waitdatavalid,
            .waitrespready = tis_waitrespready,
            .sha1threshold = 100 * 1024,
        },
};

typedef struct {
    u8            tpm_probed:1;
    u8            tpm_found:1;
    u8            tpm_working:1;
    u8            if_shutdown:1;
    u8            tpm_driver_to_use:4;
} tcpa_state_t;


static tcpa_state_t tcpa_state = {
    .tpm_driver_to_use = TPM_INVALID_DRIVER,
};

static u32
is_tpm_present(void)
{
    u32 rc = 0;
    unsigned int i;

    for (i = 0; i < TPM_NUM_DRIVERS; i++) {
        struct tpm_driver *td = &tpm_drivers[i];
        if (td->probe() != 0) {
            td->init();
            tcpa_state.tpm_driver_to_use = i;
            rc = 1;
            break;
        }
    }

    return rc;
}

int
vtpm4hvm_setup(void)
{
    if (!tcpa_state.tpm_probed) {
        tcpa_state.tpm_probed = 1;
        tcpa_state.tpm_found = (is_tpm_present() != 0);
        tcpa_state.tpm_working = 1;
    }
    if (!tcpa_state.tpm_working)
        return 0;

    return tcpa_state.tpm_found;
}

