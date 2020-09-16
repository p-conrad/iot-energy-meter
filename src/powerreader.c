#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <dal/adi_application_interface.h>

#include "utils.h"

//-----------------------------------------------------------------------------
// defines and test setup
//-----------------------------------------------------------------------------
#define KBUS_MAINPRIO 40       // main loop
#define RUNTIME_SECONDS 30

// TODO: This should be configurable by a script
Loglevel loglevel = LOGLEVEL_DEBUG;

/**
 * @brief Finds and intializes the KBus device. Requires the ADI to be initialized before calling.
 *
 * @param[in] adi A pointer to the initialized application interface
 * @param[out] kbusDeviceId The ID of the found KBus device
 * @retval ERROR_SUCCESS (0) on success
 */
ErrorCode find_and_initialize_kbus(tApplicationDeviceInterface* adi, tDeviceId* kbusDeviceId);

/**
 * @brief Triggers one KBus cycle.
 *
 * @param[in] adi A pointer to the initialized application interface
 * @param[out] kbusDeviceId The ID of the found KBus device
 * @retval ERROR_SUCCESS (0) on success
 */
ErrorCode trigger_cycle(tApplicationDeviceInterface* adi, tDeviceId kbusDeviceId);

int main(void) {
    tDeviceId kbusDeviceId;
    tApplicationDeviceInterface* adi;
    uint32_t taskId = 0;

    // process data
    uint8_t pd_in[4096];
    uint8_t pd_out[4096];

    // generic vars
    time_t last_t = 0, new_t;
    long unsigned runtime = 0;

    // startup info */
    printf("**************************************************\n");
    printf("***         WINNER Power Measurement           ***\n");
    printf("**************************************************\n");

    // clear process memory
    memset(pd_in, 0, sizeof(pd_in));
    memset(pd_out, 0, sizeof(pd_out));

    // connect to ADI-interface
    adi = adi_GetApplicationInterface();

    // init interface
    adi->Init();
    ErrorCode initResult = find_and_initialize_kbus(adi, &kbusDeviceId);
    if (initResult != ERROR_SUCCESS) {
        return initResult;
    }

    // run main loop for 30s
    int loops = 0;
    while (runtime < RUNTIME_SECONDS) {
        usleep(10000); // wait 10 ms

        ErrorCode triggerResult = trigger_cycle(adi, kbusDeviceId);
        if (triggerResult != ERROR_SUCCESS) {
            return triggerResult;
        }

        loops++;
        adi->WatchdogTrigger();

        // 1s tick for test output
        new_t = time(NULL);
        if (new_t != last_t) {
            last_t = new_t;
            runtime++;

            // read inputs
            adi->ReadStart(kbusDeviceId, taskId);     /* lock PD-In data */
            adi->ReadBytes(kbusDeviceId, taskId, 0, 1, (uint8_t *) &pd_in[0]);  /* read 1 byte from address 0 */
            adi->ReadEnd(kbusDeviceId, taskId); /* unlock PD-In data */
            // calculate something
            pd_out[0] += 1;
            // write outputs
            adi->WriteStart(kbusDeviceId, taskId); /* lock PD-out data */
            adi->WriteBytes(kbusDeviceId,taskId,0,1,(uint8_t *) &pd_out[0]); /* write */
            adi->WriteBytes(kbusDeviceId, taskId, 0, 16, (uint8_t *) &pd_out[0]); /* write */
            adi->WriteEnd(kbusDeviceId, taskId); /* unlock PD-out data */
            // print info
            printf("%lu:%02lu:%02lu State = ",runtime/3600ul,(runtime/60ul)%60ul,runtime%60ul);
            // show loops/s
            printf("\n        Loop/s = %i ",loops);
            loops = 0;
            // show process data
            printf(" Input Data = %02X Output data = %02X ",(int) pd_in[0],(int) pd_out[0]);
            printf("\n");
        }
    }

    adi->CloseDevice(kbusDeviceId);
    adi->Exit();
    return ERROR_SUCCESS;
}

ErrorCode find_and_initialize_kbus(tApplicationDeviceInterface* adi, tDeviceId* kbusDeviceId) {
    struct sched_param s_param;
    tDeviceInfo deviceList[10];
    size_t nrDevicesFound;
    tApplicationStateChangedEvent event;

    // scan devices
    adi->ScanDevices();
    adi->GetDeviceList(sizeof(deviceList), deviceList, &nrDevicesFound);

    size_t nrKbusFound = -1;
    for (int i = 0; i < nrDevicesFound; ++i) {
        if (strcmp(deviceList[i].DeviceName, "libpackbus") == 0) {
            nrKbusFound = i;
            dprintf(LOGLEVEL_DEBUG, "KBUS device found as device %i\n", i);
        }
    }

    // kbus not found > exit
    if (nrKbusFound == -1) {
        dprintf(LOGLEVEL_ERR, "No KBUS device found \n");
        adi->Exit();
        return ERROR_KBUS_NOT_FOUND;
    }

    // switch to RT Priority
    s_param.sched_priority = KBUS_MAINPRIO;
    sched_setscheduler(0, SCHED_FIFO, &s_param);
    dprintf(LOGLEVEL_NOTICE, "Scheduling priority set to %d.", KBUS_MAINPRIO);

    // open kbus device
    *kbusDeviceId = deviceList[nrKbusFound].DeviceId;
    if (adi->OpenDevice(*kbusDeviceId) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Kbus device open failed\n");
        adi->Exit(); // disconnect ADI-Interface
        return ERROR_KBUS_OPEN_FAILED; // exit program
    }
    dprintf(LOGLEVEL_NOTICE, "KBUS device opened\n");

    // Set application state to "Running" to drive kbus by your selve.
    event.State = ApplicationState_Running;
    if (adi->ApplicationStateChanged(event) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Set application state to 'Running' failed\n");
        adi->CloseDevice(*kbusDeviceId);
        adi->Exit();
        return ERROR_STATE_CHANGE_FAILED;
    }
    dprintf(LOGLEVEL_NOTICE, "Application state set to 'running'\n");

    return ERROR_SUCCESS;
}

ErrorCode trigger_cycle(tApplicationDeviceInterface* adi, tDeviceId kbusDeviceId) {
        uint32_t pushRetval = 0;

        if (adi->CallDeviceSpecificFunction("libpackbus_Push", &pushRetval) != DAL_SUCCESS) {
            dprintf(LOGLEVEL_ERR, "CallDeviceSpecificFunction failed\n");
            adi->CloseDevice(kbusDeviceId);
            adi->Exit();
            return ERROR_DEVICE_SPECIFIC_FUNCTION_FAILED;
        }

        if (pushRetval != DAL_SUCCESS) {
            dprintf(LOGLEVEL_ERR, "Function 'libpackbus_Push' failed\n");
            adi->CloseDevice(kbusDeviceId);
            adi->Exit();
            return ERROR_LIBPACKBUS_PUSH_FAILED;
        }

    return ERROR_SUCCESS;
}
