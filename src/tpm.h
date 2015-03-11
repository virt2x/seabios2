#ifndef TPM_DRIVERS_H
#define TPM_DRIVERS_H

#include "types.h" // u32


enum tpmDurationType {
    TPM_DURATION_TYPE_SHORT = 0,
    TPM_DURATION_TYPE_MEDIUM,
    TPM_DURATION_TYPE_LONG,
};

/* low level driver implementation */
struct tpm_driver {
    u32 *timeouts;
    u32 *durations;
    void (*set_timeouts)(u32 timeouts[4], u32 durations[3]);
    u32 (*probe)(void);
    u32 (*init)(void);
    u32 (*activate)(u8 locty);
    u32 (*ready)(void);
    u32 (*senddata)(const u8 *const data, u32 len);
    u32 (*readresp)(u8 *buffer, u32 *len);
    u32 (*waitdatavalid)(void);
    u32 (*waitrespready)(enum tpmDurationType to_t);
    /* the TPM will be used for buffers of sizes below the sha1threshold
       for calculating the hash */
    u32 sha1threshold;
};

extern struct tpm_driver tpm_drivers[];


#define TIS_DRIVER_IDX       0
#define TPM_NUM_DRIVERS      1

#define TPM_INVALID_DRIVER  -1

/* TIS driver */
/* address of locality 0 (TIS) */
#define TPM_TIS_BASE_ADDRESS        0xfed40000

#define TIS_REG(LOCTY, REG) \
    (void *)(TPM_TIS_BASE_ADDRESS + (LOCTY << 12) + REG)

/* hardware registers */
#define TIS_REG_ACCESS                 0x0
#define TIS_REG_INT_ENABLE             0x8
#define TIS_REG_INT_VECTOR             0xc
#define TIS_REG_INT_STATUS             0x10
#define TIS_REG_INTF_CAPABILITY        0x14
#define TIS_REG_STS                    0x18
#define TIS_REG_DATA_FIFO              0x24
#define TIS_REG_DID_VID                0xf00
#define TIS_REG_RID                    0xf04

#define TIS_STS_VALID                  (1 << 7) /* 0x80 */
#define TIS_STS_COMMAND_READY          (1 << 6) /* 0x40 */
#define TIS_STS_TPM_GO                 (1 << 5) /* 0x20 */
#define TIS_STS_DATA_AVAILABLE         (1 << 4) /* 0x10 */
#define TIS_STS_EXPECT                 (1 << 3) /* 0x08 */
#define TIS_STS_RESPONSE_RETRY         (1 << 1) /* 0x02 */

#define TIS_ACCESS_TPM_REG_VALID_STS   (1 << 7) /* 0x80 */
#define TIS_ACCESS_ACTIVE_LOCALITY     (1 << 5) /* 0x20 */
#define TIS_ACCESS_BEEN_SEIZED         (1 << 4) /* 0x10 */
#define TIS_ACCESS_SEIZE               (1 << 3) /* 0x08 */
#define TIS_ACCESS_PENDING_REQUEST     (1 << 2) /* 0x04 */
#define TIS_ACCESS_REQUEST_USE         (1 << 1) /* 0x02 */
#define TIS_ACCESS_TPM_ESTABLISHMENT   (1 << 0) /* 0x01 */

#define SCALER 10

#define TIS_DEFAULT_TIMEOUT_A          (750  * SCALER)
#define TIS_DEFAULT_TIMEOUT_B          (2000 * SCALER)
#define TIS_DEFAULT_TIMEOUT_C          (750  * SCALER)
#define TIS_DEFAULT_TIMEOUT_D          (750  * SCALER)

enum tisTimeoutType {
    TIS_TIMEOUT_TYPE_A = 0,
    TIS_TIMEOUT_TYPE_B,
    TIS_TIMEOUT_TYPE_C,
    TIS_TIMEOUT_TYPE_D,
};

#define TPM_DEFAULT_DURATION_SHORT     (2000  * SCALER)
#define TPM_DEFAULT_DURATION_MEDIUM    (20000 * SCALER)
#define TPM_DEFAULT_DURATION_LONG      (60000 * SCALER)


/***************************************************
 *                     TCG BIOS                    *
 ***************************************************/
#define TPM_OK                          0x0
#define TPM_RET_BASE                    0x1
#define TCG_GENERAL_ERROR               (TPM_RET_BASE + 0x0)
#define TCG_TPM_IS_LOCKED               (TPM_RET_BASE + 0x1)
#define TCG_NO_RESPONSE                 (TPM_RET_BASE + 0x2)
#define TCG_INVALID_RESPONSE            (TPM_RET_BASE + 0x3)
#define TCG_INVALID_ACCESS_REQUEST      (TPM_RET_BASE + 0x4)
#define TCG_FIRMWARE_ERROR              (TPM_RET_BASE + 0x5)
#define TCG_INTEGRITY_CHECK_FAILED      (TPM_RET_BASE + 0x6)
#define TCG_INVALID_DEVICE_ID           (TPM_RET_BASE + 0x7)
#define TCG_INVALID_VENDOR_ID           (TPM_RET_BASE + 0x8)
#define TCG_UNABLE_TO_OPEN              (TPM_RET_BASE + 0x9)
#define TCG_UNABLE_TO_CLOSE             (TPM_RET_BASE + 0xa)
#define TCG_RESPONSE_TIMEOUT            (TPM_RET_BASE + 0xb)
#define TCG_INVALID_COM_REQUEST         (TPM_RET_BASE + 0xc)
#define TCG_INVALID_ADR_REQUEST         (TPM_RET_BASE + 0xd)
#define TCG_WRITE_BYTE_ERROR            (TPM_RET_BASE + 0xe)
#define TCG_READ_BYTE_ERROR             (TPM_RET_BASE + 0xf)
#define TCG_BLOCK_WRITE_TIMEOUT         (TPM_RET_BASE + 0x10)
#define TCG_CHAR_WRITE_TIMEOUT          (TPM_RET_BASE + 0x11)
#define TCG_CHAR_READ_TIMEOUT           (TPM_RET_BASE + 0x12)
#define TCG_BLOCK_READ_TIMEOUT          (TPM_RET_BASE + 0x13)
#define TCG_TRANSFER_ABORT              (TPM_RET_BASE + 0x14)
#define TCG_INVALID_DRV_FUNCTION        (TPM_RET_BASE + 0x15)
#define TCG_OUTPUT_BUFFER_TOO_SHORT     (TPM_RET_BASE + 0x16)
#define TCG_FATAL_COM_ERROR             (TPM_RET_BASE + 0x17)
#define TCG_INVALID_INPUT_PARA          (TPM_RET_BASE + 0x18)
#define TCG_TCG_COMMAND_ERROR           (TPM_RET_BASE + 0x19)
#define TCG_INTERFACE_SHUTDOWN          (TPM_RET_BASE + 0x20)
#define TCG_PC_TPM_NOT_PRESENT          (TPM_RET_BASE + 0x22)
#define TCG_PC_TPM_DEACTIVATED          (TPM_RET_BASE + 0x23)

#define TPM_INVALID_ADR_REQUEST          TCG_INVALID_ADR_REQUEST
#define TPM_IS_LOCKED                    TCG_TPM_IS_LOCKED
#define TPM_INVALID_DEVICE_ID            TCG_INVALID_DEVICE_ID
#define TPM_INVALID_VENDOR_ID            TCG_INVALID_VENDOR_ID
#define TPM_FIRMWARE_ERROR               TCG_FIRMWARE_ERROR
#define TPM_UNABLE_TO_OPEN               TCG_UNABLE_TO_OPEN
#define TPM_UNABLE_TO_CLOSE              TCG_UNABLE_TO_CLOSE
#define TPM_INVALID_RESPONSE             TCG_INVALID_RESPONSE
#define TPM_RESPONSE_TIMEOUT             TCG_RESPONSE_TIMEOUT
#define TPM_INVALID_ACCESS_REQUEST       TCG_INVALID_ACCESS_REQUEST
#define TPM_TRANSFER_ABORT               TCG_TRANSFER_ABORT
#define TPM_GENERAL_ERROR                TCG_GENERAL_ERROR

int vtpm4hvm_setup(void);

#endif /* TPM_DRIVERS_H */
