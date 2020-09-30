#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <dal/adi_application_interface.h>

#include "utils.h"
#include "kbusinfo.h"
#include "collection.h"
#include "unit_description.h"

//-----------------------------------------------------------------------------
// defines and test setup
//-----------------------------------------------------------------------------
#define KBUS_MAINPRIO 40
#define CYCLE_TIME_US 50000

// TODO: This should be configurable by a script
Loglevel loglevel = LOGLEVEL_DEBUG;

// Signal handling
volatile sig_atomic_t running = 1;

/**
 * @brief The signal handler for catching the SIGINT signal.
 *
 * @param[in] signum The signal received
 */
void sig_handler(int signum) {
    if (signum == SIGINT) {
        dprintf(LOGLEVEL_NOTICE, "Received signal SIGINT, quitting...\n");
        running = 0;
    }
}

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
    tKbusInput inputData;
    tKbusOutput outputData;

    // generic vars
    time_t last_t = 0, new_t;

    printf("**************************************************\n");
    printf("***         WINNER Power Measurement           ***\n");
    printf("**************************************************\n");

    // register signal handler
    signal(SIGINT, sig_handler);

    // clear process memory
    memset(&inputData, 0, sizeof(tKbusInput));
    memset(&outputData, 0, sizeof(tKbusOutput));

    adi = adi_GetApplicationInterface();

    adi->Init();
    ErrorCode initResult = find_and_initialize_kbus(adi, &kbusDeviceId);
    if (initResult != ERROR_SUCCESS) {
        return initResult;
    }

    struct timespec startTime, finishTime;
    while (running) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);

        ErrorCode triggerResult = trigger_cycle(adi, kbusDeviceId);
        if (triggerResult != ERROR_SUCCESS) {
            return triggerResult;
        }

        adi->WatchdogTrigger();

        // 1s tick for test output
        new_t = time(NULL);

        // request A/C values of phase 1
        outputData.t495Output.commMethod = COMM_PROCESS_DATA;
        outputData.t495Output.statusRequest = STATUS_L1;
        outputData.t495Output.colID = AC_MEASUREMENT;

        UnitDescription* measurements[] = {
            &RMSVoltageL1N,
            &EffectivePowerL1,
            &ReactivePowerN1,
            &RMSCurrentL1
        };

        // TODO: This is a proof of concept only. In the future we want to be able to handle a larger list of
		// measurements which might not be a multiple 4, where we take 4 UnitDescriptions in each cycle
        for (int i = 0; i < 4; i++) {
            outputData.t495Output.metID[i] = measurements[i]->metID;
        }

        // write outputs
        adi->WriteStart(kbusDeviceId, taskId);
        adi->WriteBytes(kbusDeviceId, taskId, 0, sizeof(tKbusOutput), (void *) &outputData);
        adi->WriteEnd(kbusDeviceId, taskId);

        // read inputs
        adi->ReadStart(kbusDeviceId, taskId);
        adi->ReadBytes(kbusDeviceId, taskId, 0, sizeof(tKbusInput), (void *) &inputData);
        adi->ReadEnd(kbusDeviceId, taskId);

        if (new_t != last_t) {
            last_t = new_t;
            // show process data
            printf("\nErrors (generic, L1, L2, L3): %u %u %u %u, unstable: %u",
                   inputData.t495Input.genericError,
                   inputData.t495Input.l1Error,
                   inputData.t495Input.l2Error,
                   inputData.t495Input.l3Error,
                   inputData.t495Input.valuesUnstable
                  );
			for (int i = 0; i < 4; i++) {
				UnitDescription *currentUnit = measurements[i];
				printf("\n%s: %.2f%s",
					   currentUnit->description,
					   read_measurement_value(currentUnit, inputData.t495Input.processValue[i]),
					   currentUnit->unit
					  );
			}
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
    dprintf(LOGLEVEL_NOTICE, "Scheduling priority set to %d.\n", KBUS_MAINPRIO);

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
