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

#include "MQTTAsync.h"
#include "utils.h"
#include "kbusinfo.h"
#include "collection.h"
#include "unit_description.h"
#include "mqtt.h"

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
ErrorCode find_and_initialize_kbus(tApplicationDeviceInterface *adi, tDeviceId *kbusDeviceId);

/**
 * @brief Triggers one KBus cycle.
 *
 * @param[in] adi A pointer to the initialized application interface
 * @param[out] kbusDeviceId The ID of the found KBus device
 * @retval ERROR_SUCCESS (0) on success
 */
ErrorCode trigger_cycle(tApplicationDeviceInterface *adi, tDeviceId kbusDeviceId);

int main(void) {
    tDeviceId kbusDeviceId;
    tApplicationDeviceInterface *adi;
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

    // set the list of measurements to take and initialize the results set
    const UnitDescription *listOfMeasurements[] = {
        &RMSVoltageL1N,
        &EffectivePowerL1,
        &ReactivePowerN1,
        &RMSVoltageL2N,
        &EffectivePowerL2,
        &ReactivePowerN2,
        &RMSVoltageL3N,
        &EffectivePowerL3,
        &ReactivePowerN3
    };
    const size_t nrOfMeasurements = sizeof(listOfMeasurements) / sizeof(UnitDescription*);
    // The module can provide up to 4 measurements. If our list is shorter than that, instead of looping around we
    // simply don't fill the leftover slots.
    const size_t iMax = nrOfMeasurements >= 4 ? 4 : nrOfMeasurements;
    size_t measurementCursor = 0;

    ResultSet results = {
        .descriptions = listOfMeasurements,
        .size = nrOfMeasurements,
        .values = malloc(sizeof(double) * nrOfMeasurements),
        .validity = malloc(sizeof(bool) * nrOfMeasurements),
        .currentCount = 0
    };

    if (results.values == NULL || results.validity == NULL) {
        dprintf(LOGLEVEL_ERR, "Memory allocation failed.");
        return -ERROR_ALLOCATION_FAILED;
    }

    memset(results.values, 0, sizeof(double) * nrOfMeasurements);
    memset(results.validity, 0, sizeof(bool) * nrOfMeasurements);

    // initialize the ADI
    adi = adi_GetApplicationInterface();
    adi->Init();
    ErrorCode initResult = find_and_initialize_kbus(adi, &kbusDeviceId);
    if (initResult != ERROR_SUCCESS) {
        return initResult;
    }

    // set up MQTT
    MQTTAsync client = MQTT_init_and_connect();
    MQTTAsync_responseOptions responseOpts = MQTTAsync_responseOptions_initializer;
    responseOpts.onSuccess = on_send;
    responseOpts.onFailure = on_send_failure;
    responseOpts.context = client;

    struct timespec startTime, finishTime;
    bool outputPending = false;
    unsigned long runtimeNs = 0, remainingUs = 0;
    while (running) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);

        ErrorCode triggerResult = trigger_cycle(adi, kbusDeviceId);
        if (triggerResult != ERROR_SUCCESS) {
            return triggerResult;
        }

        adi->WatchdogTrigger();

        // 1s tick for test output
        new_t = time(NULL);

        // request A/C values and status of L1
        outputData.t495Output.commMethod = COMM_PROCESS_DATA;
        outputData.t495Output.statusRequest = STATUS_L1;
        outputData.t495Output.colID = AC_MEASUREMENT;

        for (size_t i = 0; i < iMax; i++, measurementCursor++) {
            if (measurementCursor == nrOfMeasurements) {
                measurementCursor = 0;
            }
            outputData.t495Output.metID[i] = listOfMeasurements[measurementCursor]->metID;
        }

        // write outputs
        adi->WriteStart(kbusDeviceId, taskId);
        adi->WriteBytes(kbusDeviceId, taskId, 0, sizeof(tKbusOutput), (void *) &outputData);
        adi->WriteEnd(kbusDeviceId, taskId);

        // read inputs
        adi->ReadStart(kbusDeviceId, taskId);
        adi->ReadBytes(kbusDeviceId, taskId, 0, sizeof(tKbusInput), (void *) &inputData);
        adi->ReadEnd(kbusDeviceId, taskId);

        if (results_unstable(&inputData.t495Input, iMax)) {
            goto finish_cycle;
        }

        // fill the results set
        for (size_t i = 0; i < iMax; i++) {
            size_t index;
            UnitDescription *description = find_description_with_id(results.descriptions,
                                                                    results.size,
                                                                    inputData.t495Input.metID[i],
                                                                    &index);
            if (description == NULL) continue;

            results.values[index] = read_measurement_value(description, inputData.t495Input.processValue[i]);
            if (!results.validity[index]) {
                results.validity[index] = true;
                results.currentCount += 1;
            }
        }

        // output results (roughly) every second when our results are complete
        if (new_t != last_t) {
            outputPending = true;
            last_t = new_t;
        }

        if (outputPending && results.currentCount == results.size) {
            // show process data
            printf("\nErrors (generic, L1, L2, L3): %u %u %u %u",
                   inputData.t495Input.genericError,
                   inputData.t495Input.l1Error,
                   inputData.t495Input.l2Error,
                   inputData.t495Input.l3Error
                  );
            for (size_t i = 0; i < results.size; i++) {
                printf("\n%s: %.2f%s",
                       results.descriptions[i]->description,
                       results.values[i],
                       results.descriptions[i]->unit
                      );
            }
            printf("\nLast cycle: %luus (%luus remaining)", runtimeNs / 1000, remainingUs);
            printf("\n");
            outputPending = false;
        }

        // set timestamp, do something with the finished results and then reset them
        if (results.currentCount == results.size) {
            if (MQTTAsync_isConnected(client)) {
                // Paho will handle the deallocation of messageStr for us, so we don't have to worry about it
                char *messageStr = get_MQTT_message_string(&results);
                if (messageStr == NULL) {
                    dprintf(LOGLEVEL_ERR, "Failed to allocate the MQTT result string.\n");
                    goto reset_results;
                }
                MQTTAsync_message message = MQTTAsync_message_initializer;
                message.payload = get_MQTT_message_string(&results);
                message.payloadlen = strlen(message.payload);
                message.qos = MQTT_QOS_DEFAULT;
                message.retained = 0;

                int pubResult;
                if ((pubResult = MQTTAsync_sendMessage(client, MQTT_TOPIC, &message, &responseOpts)) != MQTTASYNC_SUCCESS) {
                    printf("Failed to start sendMessage, return code %d\n", pubResult);
                    exit(EXIT_FAILURE);
                }
            }
reset_results:
            results.currentCount = 0;
            memset(results.validity, 0, sizeof(bool) * results.size);
        }

finish_cycle:
        clock_gettime(CLOCK_MONOTONIC_RAW, &finishTime);
        runtimeNs = (finishTime.tv_sec - startTime.tv_sec) * 1E9 + (finishTime.tv_nsec - startTime.tv_nsec);

        // potential bug: If the loop ever takes longer than the cycle time this will lock up
        remainingUs = CYCLE_TIME_US - (runtimeNs / 1000);
        usleep(remainingUs);
    }

    MQTT_disconnect_and_destroy(client);
    adi->CloseDevice(kbusDeviceId);
    adi->Exit();
    return ERROR_SUCCESS;
}

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
