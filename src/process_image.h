#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#include <stdbool.h>
#include <stddef.h>
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
 * @retval The int32_t read from the buffer
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
    uint8_t unusedByte0:7;
    uint8_t commMethod:1;       // COMM_METHOD

    uint8_t statusRequest:2;    // STATUS_REQUEST
    uint8_t unusedByte1:6;

    uint8_t unusedByte2;

    uint8_t colID;              // COL_ID

    uint8_t metID[4];           // MET_ID

    uint8_t unusedDataWords[16];
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
    uint8_t l1Error:1;
    uint8_t l2Error:1;
    uint8_t l3Error:1;
    uint8_t moduleError:1;
    uint8_t unusedByte0:2;
    uint8_t genericError:1;
    uint8_t commMethod:1;        // COMM_METHOD

    uint8_t statusRequest:2;     // STATUS_REQUEST
    uint8_t outOfRange4:1;
    uint8_t outOfRange3:1;
    uint8_t outOfRange2:1;
    uint8_t outOfRange1:1;
    uint8_t calMode:1;           // CAL_MODE
    uint8_t valuesUnstable:1;

    uint8_t unusedByte2:1;
    uint8_t zcUnderrun:1;
    uint8_t currentClipped:1;
    uint8_t voltageClipped:1;
    uint8_t noZeroCrossings:1;
    uint8_t overcurrent:1;
    uint8_t overvoltageOrRotaryFieldIncorect:1;
    uint8_t undervoltageOrTampered:1;

    uint8_t colID;               // COL_ID

    uint8_t metID[4];            // MET_ID

    uint8_t processValue[4][4];
} Type495ProcessInput;

/**
 * @brief Checks whether some measurements in the collection are still unstable or not available
 *
 * @param[in] input The process input image obtained from the module
 * @param[in] iMax The maximum index to check for
 *
 * @retval true if the valuesUnstable flag is set or any of the metIDs are 0, false otherwise
 */
bool results_unstable(Type495ProcessInput *input, const size_t iMax) {
    if (input->valuesUnstable) {
	return true;
    }
    for (size_t i = 0; i < iMax; i++) {
	if (input->metID[i] == 0) {
	    return true;
	}
    }
    return false;
}

#endif
