#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dal/adi_application_interface.h>
#include <ldkc_kbus_information.h>
#include <ldkc_kbus_register_communication.h>

#include "process_image.h"
#include "utils.h"

#define KBUS_MAINPRIO 40

/**
 * @brief Sets the state of the PLC application
 *
 * @param[in] adi A pointer to the initialized application interface
 * @param[in] event The event containing the desired application state
 * @retval ERROR_SUCCESS (0) on success, -ERROR_STATE_CHANGE_FAILED otherwise
 */
ErrorCode set_application_state(tApplicationDeviceInterface *adi,
                                tApplicationStateChangedEvent event) {
    // stringify the state so we get more useful debug output
    char *state;
    switch (event.State) {
        case ApplicationState_Running:
            state = "Running";
            break;
        case ApplicationState_Stopped:
            state = "Stopped";
            break;
        case ApplicationState_Unconfigured:
            state = "Unconfigured";
            break;
        default:
            state = "Invalid";
    }

    if (adi->ApplicationStateChanged(event) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Failed to set the application state to '%s'\n", state);
        return -ERROR_STATE_CHANGE_FAILED;
    }

    dprintf(LOGLEVEL_INFO, "Application state set to '%s'\n", state);
    return ERROR_SUCCESS;
}

/**
 * @brief Calculates the sizes of both the process input and output data size of the KBus.
 *        The KBus info must be created by calling ldkc_KbusInfo_Create() first.
 *
 * @param[out] inputSize The process input data size
 * @param[out] outputSize The process output data size
 * @retval ERROR_SUCCESS (0) on success, -ERROR_KBUSINFO_STATUS_FAILED otherwise
 */
ErrorCode get_process_data_size(size_t *inputSize, size_t *outputSize) {
    tldkc_KbusInfo_Status status;
    if (ldkc_KbusInfo_GetStatus(&status) == KbusInfo_Failed) {
        dprintf(LOGLEVEL_ERR, "Failed to retrieve the KBus status\n");
        ldkc_KbusInfo_Destroy();
        return -ERROR_KBUSINFO_STATUS_FAILED;
    }

    *inputSize = (status.BitCountAnalogInput / 8) + (status.BitCountDigitalInput / 8 + 1);
    *outputSize = (status.BitCountAnalogOutput / 8) + (status.BitCountDigitalOutput / 8 + 1);

    return ERROR_SUCCESS;
}

/**
 * @brief Finds the process data addresses of all power measurement modules
 *
 * @param[in] inputData A pointer to the allocated input process data
 * @param[in] outputData A pointer to the allocated output process data
 * @param[out] count The number of power measurement modules found
 * @param[out] t495Inputs A newly allocated array of pointers to all Type 495/494 process input data
 * @param[out] t495Outputs A newly allocated array of pointers to all Type 495/494 process output data
 * @retval ERROR_SUCCESS (0) on success, a different error code otherwise
 */
ErrorCode get_pm_data_addresses(const void *inputData,
                                const void *outputData,
                                size_t *count,
                                Type495ProcessInput ***t495Inputs,
                                Type495ProcessOutput ***t495Outputs) {
    size_t terminalCount;
    uint16_t terminals[LDKC_KBUS_TERMINAL_COUNT_MAX];
    tldkc_KbusInfo_TerminalInfo terminalDescription[LDKC_KBUS_TERMINAL_COUNT_MAX];

    int inputOffsets[LDKC_KBUS_TERMINAL_COUNT_MAX];
    int outputOffsets[LDKC_KBUS_TERMINAL_COUNT_MAX];
    size_t moduleCount = 0;

    if (ldkc_KbusInfo_GetTerminalInfo(OS_ARRAY_SIZE(terminalDescription),
                                      terminalDescription, &terminalCount) == KbusInfo_Failed) {
        dprintf(LOGLEVEL_ERR, "Failed to get the terminal info\n");
        ldkc_KbusInfo_Destroy();
        return -ERROR_KBUSINFO_TERMINAL_INFO_FAILED;
    }

    if (ldkc_KbusInfo_GetTerminalList(OS_ARRAY_SIZE(terminals), terminals, NULL) == KbusInfo_Failed) {
        dprintf(LOGLEVEL_ERR, "Failed to get the terminal list\n");
        ldkc_KbusInfo_Destroy();
        return -ERROR_KBUSINFO_TERMINAL_LIST_FAILED;
    }

    for (size_t i = 0; i < terminalCount; i++) {
        if (terminals[i] == 494 || terminals[i] == 495) {
            inputOffsets[moduleCount] = terminalDescription[i].OffsetInput_bits / 8;
            outputOffsets[moduleCount] = terminalDescription[i].OffsetOutput_bits / 8;
            moduleCount++;
        } else if (terminals[i] == 493) {
            dprintf(LOGLEVEL_WARNING,
                    "Found a 750-493 power measurement module. \
                    This type is not supported and will be ignored\n");
        }
    }

    if (moduleCount == 0) {
        dprintf(LOGLEVEL_ERR, "No power measurement modules found\n");
        return -ERROR_NO_MODULES;
    }

    dprintf(LOGLEVEL_INFO, "Found %u power measurement modules\n", moduleCount);

    *t495Inputs = malloc(sizeof(Type495ProcessInput*) * moduleCount);
    *t495Outputs = malloc(sizeof(Type495ProcessOutput*) * moduleCount);
    if (*t495Inputs == NULL || *t495Outputs == NULL) {
        dprintf(LOGLEVEL_ERR, "Failed to allocate memory for the module pointer addresses\n");
        return -ERROR_ALLOCATION_FAILED;
    }

    for (size_t i = 0; i < moduleCount; i++) {
        *t495Inputs[i] = (void *)inputData + inputOffsets[i];
        *t495Outputs[i] = (void *)outputData + outputOffsets[i];
    }
    *count = moduleCount;

    return ERROR_SUCCESS;
}

/**
 * @brief Finds and intializes the KBus device. Requires the ADI to be initialized before calling.
 *
 * @param[in] adi A pointer to the initialized application interface
 * @param[out] kbusDeviceId The ID of the found KBus device
 * @retval ERROR_SUCCESS (0) on success
 */
ErrorCode find_and_initialize_kbus(tApplicationDeviceInterface *adi, tDeviceId *kbusDeviceId) {
    struct sched_param s_param;
    tDeviceInfo deviceList[10];
    size_t nrDevicesFound;

    adi->ScanDevices();
    adi->GetDeviceList(sizeof(deviceList), deviceList, &nrDevicesFound);

    size_t nrKbusFound = -1;
    for (size_t i = 0; i < nrDevicesFound; ++i) {
        if (strcmp(deviceList[i].DeviceName, "libpackbus") == 0) {
            nrKbusFound = i;
            dprintf(LOGLEVEL_DEBUG, "KBUS device found as device %i\n", i);
        }
    }

    if (nrKbusFound == -1) {
        dprintf(LOGLEVEL_ERR, "No KBUS device found \n");
        return -ERROR_KBUS_NOT_FOUND;
    }

    s_param.sched_priority = KBUS_MAINPRIO;
    sched_setscheduler(0, SCHED_FIFO, &s_param);
    dprintf(LOGLEVEL_NOTICE, "Scheduling priority set to %d\n", KBUS_MAINPRIO);

    *kbusDeviceId = deviceList[nrKbusFound].DeviceId;
    if (adi->OpenDevice(*kbusDeviceId) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Kbus device open failed\n");
        return -ERROR_KBUS_OPEN_FAILED;
    }
    dprintf(LOGLEVEL_NOTICE, "KBUS device opened\n");

    return ERROR_SUCCESS;
}

/**
 * @brief Triggers one KBus cycle.
 *
 * @param[in] adi A pointer to the initialized application interface
 * @param[out] kbusDeviceId The ID of the found KBus device
 * @retval ERROR_SUCCESS (0) on success
 */
ErrorCode trigger_cycle(tApplicationDeviceInterface *adi, tDeviceId kbusDeviceId) {
    uint32_t pushRetval = 0;

    if (adi->CallDeviceSpecificFunction("libpackbus_Push", &pushRetval) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "CallDeviceSpecificFunction failed\n");
        return -ERROR_DEVICE_SPECIFIC_FUNCTION_FAILED;
    }

    if (pushRetval != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Function 'libpackbus_Push' failed\n");
        return -ERROR_LIBPACKBUS_PUSH_FAILED;
    }

    return ERROR_SUCCESS;
}
