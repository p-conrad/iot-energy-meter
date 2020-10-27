#ifndef UNIT_DESCRIPTION_H
#define UNIT_DESCRIPTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

#include "collection.h"
#include "process_image.h"
#include "utils.h"

/**
 * @brief A struct containing all necessary information for querying a
 *        measurement value.
 */
typedef struct UnitDescription {
    // this needs to somehow change if we ever want to use a different table
    const MET_ID_AC metID;
    const char *unit;
    const char *description;
    const int scalingFactor;
    const bool isUnsigned;
} UnitDescription;

/**
 * @brief A struct containing a list of UnitDescription together with their current result values, and a timestamp.
 */
typedef struct ResultSet {
    const UnitDescription **descriptions;
    const size_t size;
    const size_t moduleIndex;       /* index of the power measurement module on the bus */
    double *values;                 /* result values at the same positions as descriptions */
    struct timespec timestamp;      /* the timestamp when the set was completed */
    bool *validity;                 /* used to determine whether a certain value has already been filled */
    size_t currentCount;            /* number of valid entries to quickly know whether the set is finshed */
} ResultSet;


/**
 * @brief Allocates a list of multiple ResultSets
 *
 * @param[in] descriptions The list of UnitDescriptions to associate with the ResultSets
 * @param[in] descSize The length of the UnitDescription list
 * @param[in] The number of modules (e.g. the resulting size) to allocate the ResultSets for
 * @retval A pointer to the allocated ResultSets, or NULL on alloation failure
 */
ResultSet *allocate_results(const UnitDescription **descriptions, const size_t descSize, const size_t moduleCount) {
    ResultSet *result = calloc(sizeof(ResultSet), moduleCount);

    if (result == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < moduleCount; i++) {
        result[i].descriptions = descriptions;
        *(size_t*)&result[i].size = descSize;
        *(size_t*)&result[i].moduleIndex = i;
        result[i].values = calloc(sizeof(double), descSize);
        result[i].validity = calloc(sizeof(bool), descSize);
        result[i].currentCount = 0;

        if (result[i].values == NULL || result[i].validity == NULL) {
            // strictly speaking we would have to clean up all the allocated memory here,
            // but the program is supposed to terminate here anyway, so we save us the effort
            return NULL;
        }
    }

    return result;
}

/**
 * @brief Reads and converts a measurement value according to its unit description.
 *
 * @param[in] unit A pointer to the unit description
 * @param[in] buf The buffer from the process output image containing the value to be read
 *
 * @retval The converted and correctly scaled measurement value in double precision
 */
double read_measurement_value(const UnitDescription *unit, uint8_t *buf) {
    double result;
    if (unit->isUnsigned) {
        result = (double) read_uint32(buf);
    } else {
        result = (double) read_int32(buf);
    }
    return result / unit->scalingFactor;
}

/**
 * @brief Finds the UnitDescription with a given MET_ID in a provided list.
 *        Useful for correctly interpreting the process values from a given process input.
 *
 * @param[in] list An Array of pointers to avalable UnitDescription instances
 * @param[in] listSize The size of the provided array
 * @param[in] id The measurement ID from the process input to search for
 * @param[out] index The list index of the found description (can be NULL if not desired)
 *
 * @retval A pointer to the corresponding UnitDescription if found, NULL otherwise
 */
UnitDescription *find_description_with_id(const UnitDescription **list, size_t listSize, int id, size_t *index) {
    if (list == NULL || listSize == 0 || id == 0) {
        return NULL;
    }
    for (size_t i = 0; i < listSize; i++) {
        if (list[i]->metID == id) {
            if (index != NULL) {
                *index = i;
            }
            return (UnitDescription *)list[i];
        }
    }
    return NULL;
}

/*
 * Some commonly used measurements are defined here for convenience.
 */
UnitDescription RMSVoltageL1N = {
    .metID = VOLTAGE_RMS_L1N,
    .unit = "V",
    .description = "RMS Voltage, L1-N",
    .scalingFactor = 100,
    .isUnsigned = true
};

UnitDescription RMSVoltageL2N = {
    .metID = VOLTAGE_RMS_L2N,
    .unit = "V",
    .description = "RMS Voltage, L2-N",
    .scalingFactor = 100,
    .isUnsigned = true
};

UnitDescription RMSVoltageL3N = {
    .metID = VOLTAGE_RMS_L3N,
    .unit = "V",
    .description = "RMS Voltage, L3-N",
    .scalingFactor = 100,
    .isUnsigned = true
};

UnitDescription RMSCurrentL1 = {
    .metID = CURRENT_RMS_L1,
    .unit = "A",
    .description = "RMS current, L1",
    .scalingFactor = 10000,
    .isUnsigned = true
};

UnitDescription RMSCurrentL2 = {
    .metID = CURRENT_RMS_L2,
    .unit = "A",
    .description = "RMS current, L2",
    .scalingFactor = 10000,
    .isUnsigned = true
};

UnitDescription RMSCurrentL3 = {
    .metID = CURRENT_RMS_L3,
    .unit = "A",
    .description = "RMS current, L3",
    .scalingFactor = 10000,
    .isUnsigned = true
};

UnitDescription RMSCurrentN = {
    .metID = CURRENT_RMS_N,
    .unit = "A",
    .description = "RMS current, N",
    .scalingFactor = 10000,
    .isUnsigned = true
};

UnitDescription EffectivePowerL1 = {
    .metID = POWER_EFFECTIVE_L1,
    .unit = "W",
    .description = "Effective Power, L1",
    .scalingFactor = 100,
    .isUnsigned = false
};

UnitDescription EffectivePowerL2 = {
    .metID = POWER_EFFECTIVE_L2,
    .unit = "W",
    .description = "Effective Power, L2",
    .scalingFactor = 100,
    .isUnsigned = false
};

UnitDescription EffectivePowerL3 = {
    .metID = POWER_EFFECTIVE_L3,
    .unit = "W",
    .description = "Effective Power, L3",
    .scalingFactor = 100,
    .isUnsigned = false
};

UnitDescription ReactivePowerN1 = {
    .metID = POWER_REACTIVE_L1,
    .unit = "VAR",
    .description = "Reactive Power, L1",
    .scalingFactor = 100,
    .isUnsigned = false
};

UnitDescription ReactivePowerN2 = {
    .metID = POWER_REACTIVE_L2,
    .unit = "VAR",
    .description = "Reactive Power, L2",
    .scalingFactor = 100,
    .isUnsigned = false
};

UnitDescription ReactivePowerN3 = {
    .metID = POWER_REACTIVE_L3,
    .unit = "VAR",
    .description = "Reactive Power, L3",
    .scalingFactor = 100,
    .isUnsigned = false
};

UnitDescription ApparentPowerL1 = {
    .metID = POWER_APPARENT_L1,
    .unit = "VAR",
    .description = "Apparent Power, L1",
    .scalingFactor = 100,
    .isUnsigned = true
};

UnitDescription ApparentPowerL2 = {
    .metID = POWER_APPARENT_L2,
    .unit = "VAR",
    .description = "Apparent Power, L2",
    .scalingFactor = 100,
    .isUnsigned = true
};

UnitDescription ApparentPowerL3 = {
    .metID = POWER_APPARENT_L3,
    .unit = "VAR",
    .description = "Apparent Power, L3",
    .scalingFactor = 100,
    .isUnsigned = true
};

#endif
