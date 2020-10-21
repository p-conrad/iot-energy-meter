#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dal/adi_application_interface.h>
#include <ldkc_kbus_information.h>

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
