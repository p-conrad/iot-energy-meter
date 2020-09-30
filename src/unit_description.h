#ifndef UNIT_DESCRIPTION_H
#define UNIT_DESCRIPTION_H

#include "stdbool.h"
#include "collection.h"
#include "process_image.h"

/*
 * @brief A struct containing all necessary information for querying a
 *        measurement value.
 */
typedef struct UnitDescription {
    // TODO: This needs to somehow change if we ever want to use a different table
    const MET_ID_AC metID;
    const char* unit;
    const char* description;
    const int scalingFactor;
    const bool isUnsigned;
} UnitDescription;

/*
 * @brief Reads and converts a measurement value according to its unit description.
 *
 * @param[in] unit A pointer to the unit description
 * @param[in] buf The buffer from the process output image containing the value to be read
 *
 * @retval The converted and correctly scaled measurement value in double precision
 */
double read_measurement_value(UnitDescription *unit, unsigned char *buf) {
    double result;
    if (unit->isUnsigned) {
        result = (double) read_uint32(buf);
    } else {
        result = (double) read_int32(buf);
    }
    return result / unit->scalingFactor;
}

/*
 * Some commonly used measurements are defined here for convenience.
 */
 UnitDescription RMSVoltageL1N = {
    .metID = VOLTAGE_RMS_L1N,
    .unit = "V",
    .description = "RMS voltage, L1-N",
    .scalingFactor = 100,
    .isUnsigned = true
};

 UnitDescription RMSVoltageL2N = {
    .metID = VOLTAGE_RMS_L2N,
    .unit = "V",
    .description = "RMS voltage, L2-N",
    .scalingFactor = 100,
    .isUnsigned = true
};

 UnitDescription RMSVoltageL3N = {
    .metID = VOLTAGE_RMS_L3N,
    .unit = "V",
    .description = "RMS voltage, L3-N",
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
