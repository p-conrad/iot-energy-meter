#include <sched.h>
#include <stdio.h>
#include <string.h>

#include <dal/adi_application_interface.h>

#include "utils.h"

#define KBUS_MAINPRIO 40

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
    tApplicationStateChangedEvent event;

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
        adi->Exit();
        return -ERROR_KBUS_NOT_FOUND;
    }

    s_param.sched_priority = KBUS_MAINPRIO;
    sched_setscheduler(0, SCHED_FIFO, &s_param);
    dprintf(LOGLEVEL_NOTICE, "Scheduling priority set to %d.\n", KBUS_MAINPRIO);

    *kbusDeviceId = deviceList[nrKbusFound].DeviceId;
    if (adi->OpenDevice(*kbusDeviceId) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Kbus device open failed\n");
        adi->Exit();
        return -ERROR_KBUS_OPEN_FAILED;
    }
    dprintf(LOGLEVEL_NOTICE, "KBUS device opened\n");

    event.State = ApplicationState_Running;
    if (adi->ApplicationStateChanged(event) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Set application state to 'Running' failed\n");
        adi->CloseDevice(*kbusDeviceId);
        adi->Exit();
        return -ERROR_STATE_CHANGE_FAILED;
    }
    dprintf(LOGLEVEL_NOTICE, "Application state set to 'running'\n");

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
        adi->CloseDevice(kbusDeviceId);
        adi->Exit();
        return -ERROR_DEVICE_SPECIFIC_FUNCTION_FAILED;
    }

    if (pushRetval != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Function 'libpackbus_Push' failed\n");
        adi->CloseDevice(kbusDeviceId);
        adi->Exit();
        return -ERROR_LIBPACKBUS_PUSH_FAILED;
    }

    return ERROR_SUCCESS;
}
