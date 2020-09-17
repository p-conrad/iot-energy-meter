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
#include "kbusinfo.h"

//-----------------------------------------------------------------------------
// defines and test setup
//-----------------------------------------------------------------------------
#define KBUS_MAINPRIO 40
#define RUNTIME_SECONDS 30
#define CYCLE_TIME_US 50000

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
    uint8_t kbusInputData[sizeof(tKbusInput)];
    uint8_t kbusOutputData[sizeof(tKbusOutput)];

    // generic vars
    time_t last_t = 0, new_t;
    long unsigned runtime = 0;

    // startup info */
    printf("**************************************************\n");
    printf("***         WINNER Power Measurement           ***\n");
    printf("**************************************************\n");

    // clear process memory
    memset(kbusInputData, 0, sizeof(kbusInputData));
    memset(kbusOutputData, 0, sizeof(kbusOutputData));

    adi = adi_GetApplicationInterface();

    adi->Init();
    ErrorCode initResult = find_and_initialize_kbus(adi, &kbusDeviceId);
    if (initResult != ERROR_SUCCESS) {
        return initResult;
    }

    // run main loop for 30s
    int loops = 0;
	struct timespec startTime, finishTime;
    while (runtime < RUNTIME_SECONDS) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);

        ErrorCode triggerResult = trigger_cycle(adi, kbusDeviceId);
        if (triggerResult != ERROR_SUCCESS) {
            return triggerResult;
        }

        loops++;
        adi->WatchdogTrigger();

        // 1s tick for test output
        new_t = time(NULL);

		// read inputs
		adi->ReadStart(kbusDeviceId, taskId);
		adi->ReadBytes(kbusDeviceId, taskId, 0, sizeof(tKbusInput), kbusInputData);
		adi->ReadEnd(kbusDeviceId, taskId);

		// write outputs
		adi->WriteStart(kbusDeviceId, taskId);
		adi->WriteBytes(kbusDeviceId, taskId, 0, sizeof(tKbusOutput), kbusOutputData);
		adi->WriteEnd(kbusDeviceId, taskId);

        if (new_t != last_t) {
            last_t = new_t;
            runtime++;
            // show process data
            tKbusInput* structuredInputData = (tKbusInput*)kbusInputData;
            printf("\nStatus bytes [0..3]: %X %X %X %X",
                   (uint8_t)(structuredInputData->p3t495c1[0]),
                   (uint8_t)(structuredInputData->p3t495c1[1]),
                   (uint8_t)(structuredInputData->p3t495c1[2]),
                   (uint8_t)(structuredInputData->p3t495c1[3])
                  );
            printf("\nDigital inputs: %u %u", structuredInputData->p1t4XXc1, structuredInputData->p1t4XXc2);
            printf("\n");
        }

		clock_gettime(CLOCK_MONOTONIC_RAW, &finishTime);
		unsigned long runtimeNs = (finishTime.tv_sec - startTime.tv_sec) * 1E9 + (finishTime.tv_nsec - startTime.tv_nsec);

		// potential bug: If the loop ever takes longer than the cycle time this will lock up
		unsigned long remainingUs = CYCLE_TIME_US - (runtimeNs / 1000);
		usleep(remainingUs);
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

    adi->ScanDevices();
    adi->GetDeviceList(sizeof(deviceList), deviceList, &nrDevicesFound);

    size_t nrKbusFound = -1;
    for (int i = 0; i < nrDevicesFound; ++i) {
        if (strcmp(deviceList[i].DeviceName, "libpackbus") == 0) {
            nrKbusFound = i;
            dprintf(LOGLEVEL_DEBUG, "KBUS device found as device %i\n", i);
        }
    }

    if (nrKbusFound == -1) {
        dprintf(LOGLEVEL_ERR, "No KBUS device found \n");
        adi->Exit();
        return ERROR_KBUS_NOT_FOUND;
    }

    s_param.sched_priority = KBUS_MAINPRIO;
    sched_setscheduler(0, SCHED_FIFO, &s_param);
    dprintf(LOGLEVEL_NOTICE, "Scheduling priority set to %d.", KBUS_MAINPRIO);

    *kbusDeviceId = deviceList[nrKbusFound].DeviceId;
    if (adi->OpenDevice(*kbusDeviceId) != DAL_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Kbus device open failed\n");
        adi->Exit();
        return ERROR_KBUS_OPEN_FAILED;
    }
    dprintf(LOGLEVEL_NOTICE, "KBUS device opened\n");

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
