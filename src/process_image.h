#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

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
 * @brief The collection ID to query measurement values from
 */
typedef enum COL_ID {
    AC_MEASUREMENT = 10,
    HARMONIC_ANALYSIS_L1 = 20,
    HARMONIC_ANALYSIS_L2 = 21,
    HARMONIC_ANALYSIS_L3 = 22
} COL_ID;

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
} type495processOutput;

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
} type495processInput;

#endif
