#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#include <stdint.h>

/**
 * @brief Reads a uint32 from a buffer where bytes are in ascending order.
 *
 * @param[in] buf The buffer to read from
 * @retval The uint32_t read from the buffer
 */
uint32_t read_uint32(uint8_t *buf) {
    uint32_t result = 0;
    result |= buf[3] << 24;
    result |= buf[2] << 16;
    result |= buf[1] << 8;
    result |= buf[0];
    return result;
}

/**
 * @brief Reads a int32 from a buffer at a given offset where bytes are in ascending order.
 *
 * @param[in] buf The buffer to read from
 * @param[in] offset The offset in buf to read from
 * @retval The uint32_t read from the buffer
 */
int32_t read_int32(uint8_t *buf) {
	return read_uint32(buf);
}


/**
 * @brief The communication method used for the process image
 */
typedef enum COMM_METHOD {
    COMM_PROCESS_DATA = 0,
    COMM_REGISTER = 1,
} COMM_METHOD;

/**
 * @brief The status requested in the process output data
 */
typedef enum STATUS_REQUEST {
    STATUS_L1 = 0,
    STATUS_L2 = 1,
    STATUS_L3 = 3,
    STATUS_MOD = 4
} STATUS_REQUEST;

/**
 * @brief A struct representing the complete process output image of type 494/495 power measurement modules
 */
typedef struct __attribute__((packed)) {
    unsigned char unusedByte0:7;
    unsigned char commMethod:1;       // COMM_METHOD

    unsigned char statusRequest:2;    // STATUS_REQUEST
    unsigned char unusedByte1:6;

    unsigned char unusedByte2;

    unsigned char colID;              // COL_ID

    unsigned char metID[4];           // MET_ID

    unsigned char unusedDataWords[16];
} Type495ProcessOutput;

/**
 * @brief An indicator in the process input data showing the current mode of the module
 */
typedef enum CAL_MODE {
    MEASUREMENT_MODE = 0,
    CALIBRATION_MODE = 1
} CAL_MODE;

/**
 * @brief A struct representing the complete process input image of type 494/495 power measurement modules
 */
typedef struct __attribute__((packed)) {
    unsigned char l1Error:1;
    unsigned char l2Error:1;
    unsigned char l3Error:1;
    unsigned char moduleError:1;
    unsigned char unusedByte0:2;
    unsigned char genericError:1;
    unsigned char commMethod:1;        // COMM_METHOD

    unsigned char statusRequest:2;     // STATUS_REQUEST
    unsigned char outOfRange4:1;
    unsigned char outOfRange3:1;
    unsigned char outOfRange2:1;
    unsigned char outOfRange1:1;
    unsigned char calMode:1;           // CAL_MODE
    unsigned char valuesUnstable:1;

    unsigned char unusedByte2:1;
    unsigned char zcUnderrun:1;
    unsigned char currentClipped:1;
    unsigned char voltageClipped:1;
    unsigned char noZeroCrossings:1;
    unsigned char overcurrent:1;
    unsigned char overvoltageOrRotaryFieldIncorect:1;
    unsigned char undervoltageOrTampered:1;

    unsigned char colID;               // COL_ID

    unsigned char metID[4];            // MET_ID

    unsigned char processValue[4][4];
} Type495ProcessInput;

#endif
