#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <dal/adi_application_interface.h>
#include <MQTTAsync.h>

#include "utils.h"
#include "kbus.h"
#include "collection.h"
#include "unit_description.h"
#include "mqtt.h"

//-----------------------------------------------------------------------------
// defines and test setup
//-----------------------------------------------------------------------------
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

int main(void) {
    tDeviceId kbusDeviceId;
    tApplicationDeviceInterface *adi;
    uint32_t taskId = 0;
    tApplicationStateChangedEvent event;

    printf("**************************************************\n");
    printf("***         WINNER Power Measurement           ***\n");
    printf("**************************************************\n");

    // initialize the ADI and find the process data size
    adi = adi_GetApplicationInterface();
    adi->Init();
    exit_on_error(find_and_initialize_kbus(adi, &kbusDeviceId));

    event.State = ApplicationState_Unconfigured;
    exit_on_error(set_application_state(adi, event));

    if (ldkc_KbusInfo_Create() == KbusInfo_Failed) {
        printf("Failed to create KBus info\n");
        adi->CloseDevice(kbusDeviceId);
        adi->Exit();
        return -ERROR_KBUSINFO_CREATE_FAILED;
    }

    size_t inputDataSize, outputDataSize;
    exit_on_error(get_process_data_size(&inputDataSize, &outputDataSize));
    dprintf(LOGLEVEL_INFO, "Input/output data sizes: %u %u\n", inputDataSize, outputDataSize);

    // allocate and clear process image memory
    void *inputData = malloc(inputDataSize);
    void *outputData = malloc(outputDataSize);
    if (inputData == NULL || outputData == NULL) {
        dprintf(LOGLEVEL_ERR, "Failed to allocate memory for the process images\n");
        return -ERROR_ALLOCATION_FAILED;
    }
    memset(inputData, 0, inputDataSize);
    memset(outputData, 0, outputDataSize);

    // get the count and process data addresses of all power measurement modules
    size_t pmModuleCount;
    Type495ProcessInput **t495Inputs;
    Type495ProcessOutput **t495Outputs;
    exit_on_error(get_pm_data_addresses(inputData, outputData, &pmModuleCount, &t495Inputs, &t495Outputs));

    // finish using the KBus DBus interface
    ldkc_KbusInfo_Destroy();

    // register signal handler
    signal(SIGINT, sig_handler);

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
    // The module can provide up to 4 measurements. If our list is shorter than that,
    // instead of looping around we simply don't fill the leftover slots.
    const size_t iMax = nrOfMeasurements >= 4 ? 4 : nrOfMeasurements;
    size_t measurementCursor = 0;

    ResultSet *results = allocate_results(listOfMeasurements,
                                          nrOfMeasurements,
                                          pmModuleCount);
    if (results == NULL) {
        dprintf(LOGLEVEL_ERR, "Memory allocation for the result set failed\n");
        return -ERROR_ALLOCATION_FAILED;
    }

    // set up MQTT
    MQTTAsync client = MQTT_init_and_connect();
    MQTTAsync_responseOptions responseOpts = MQTTAsync_responseOptions_initializer;
    responseOpts.onSuccess = on_send;
    responseOpts.onFailure = on_send_failure;
    responseOpts.context = client;

    // set the application state to 'running' and start the main loop
    event.State = ApplicationState_Running;
    exit_on_error(set_application_state(adi, event));

    struct timespec startTime, finishTime;
    unsigned long runtimeNs = 0, remainingUs = 0;
    while (running) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);
        exit_on_error(trigger_cycle(adi, kbusDeviceId));
        adi->WatchdogTrigger();

        // read inputs
        adi->ReadStart(kbusDeviceId, taskId);
        adi->ReadBytes(kbusDeviceId, taskId, 0, inputDataSize, inputData);
        adi->ReadEnd(kbusDeviceId, taskId);

        // iterate through the process data of each module and process the data
        for (size_t modIndex = 0; modIndex < pmModuleCount; modIndex++) {
            if (results_unstable(t495Inputs[modIndex], iMax)) {
                continue;
            }

            // fill the results set
            for (size_t i = 0; i < iMax; i++) {
                size_t index;
                UnitDescription *description = find_description_with_id(results[modIndex].descriptions,
                                                                        results[modIndex].size,
                                                                        t495Inputs[modIndex]->metID[i],
                                                                        &index);
                if (description == NULL) continue;

                results[modIndex].values[index] = read_measurement_value(description,
                                                                         t495Inputs[modIndex]->processValue[i]);
                if (!results[modIndex].validity[index]) {
                    results[modIndex].validity[index] = true;
                    results[modIndex].currentCount += 1;
                }
            }

            // send the finished results and then reset them
            if (results[modIndex].currentCount == results[modIndex].size) {
                clock_gettime(CLOCK_TAI, &results[modIndex].timestamp);
                if (MQTTAsync_isConnected(client)) {
                    // Paho will handle the deallocation of messageStr for us, so we don't have to worry about it
                    char *messageStr = get_MQTT_message_string(&results[modIndex]);
                    if (messageStr == NULL) {
                        dprintf(LOGLEVEL_ERR, "Failed to allocate the MQTT result string\n");
                        goto reset_results;
                    }
                    MQTTAsync_message message = MQTTAsync_message_initializer;
                    message.payload = messageStr;
                    message.payloadlen = strlen(message.payload);
                    message.qos = MQTT_QOS_DEFAULT;
                    message.retained = 0;

                    int pubResult;
                    if ((pubResult = MQTTAsync_sendMessage(client, MQTT_TOPIC, &message, &responseOpts)) != MQTTASYNC_SUCCESS) {
                        printf("Failed to start sendMessage, return code %d\n", pubResult);
                    }
                }
reset_results:
                results[modIndex].currentCount = 0;
                memset(results[modIndex].validity, 0, sizeof(bool) * results[modIndex].size);
                memset(&results[modIndex].timestamp, 0, sizeof(struct timespec));
            }

            // request A/C values and status of L1
            t495Outputs[modIndex]->commMethod = COMM_PROCESS_DATA;
            t495Outputs[modIndex]->statusRequest = STATUS_L1;
            t495Outputs[modIndex]->colID = AC_MEASUREMENT;
        }

        // request the next batch of measurements - this needs to be done in a separate loop to
        // ensure we request the same values from each module
        for (size_t i = 0; i < iMax; i++, measurementCursor++) {
            if (measurementCursor == nrOfMeasurements) {
                measurementCursor = 0;
            }
            for (size_t modIndex = 0; modIndex < pmModuleCount; modIndex++) {
                t495Outputs[modIndex]->metID[i] = listOfMeasurements[measurementCursor]->metID;
            }
        }

        // write outputs
        adi->WriteStart(kbusDeviceId, taskId);
        adi->WriteBytes(kbusDeviceId, taskId, 0, outputDataSize, outputData);
        adi->WriteEnd(kbusDeviceId, taskId);

        // measure the runtime and sleep until the cycle time has elapsed
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
