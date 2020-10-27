#ifndef MQTT_H
#define MQTT_H

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "MQTTAsync.h"
#include "collection.h"
#include "unit_description.h"
#include "utils.h"
#include "protobuf/result_set.pb-c.h"

/* MQTT callbacks */
void on_connect_success(void *context, MQTTAsync_successData5 *response) {
    dprintf(LOGLEVEL_INFO, "Connection to the MQTT broker successful\n");
}

void on_connect_failure(void *context, MQTTAsync_failureData5 *response) {
    dprintf(LOGLEVEL_ERR,
            "Connection to the MQTT broker failed, response code: %d\n",
            response->code);
}

void on_disconnect(void *context, MQTTAsync_successData5 *response) {
    dprintf(LOGLEVEL_INFO, "Successfully disconnected\n");
}

void on_disconnect_failure(void *context, MQTTAsync_failureData5 *response) {
    dprintf(LOGLEVEL_ERR,
            "Disconnection failed. response code: %d\n",
            response->code);
}

void on_connection_lost(void *context, char *cause) {
    // automatic reconnection is set in the connection options and should happen
    // without taking any action here
    dprintf(LOGLEVEL_WARNING,
            "Connection lost (cause: %s), trying to reconnect...\n",
            cause);
}

int on_message_arrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    dprintf(LOGLEVEL_DEBUG,
            "Message arrived on topic %s: %.*s",
            topicName, message->payloadlen, (char *)message->payload);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void on_send(void *context, MQTTAsync_successData5 *response) {
    dprintf(LOGLEVEL_DEBUG,
            "Message with token value %d delivery confirmed\n",
            response->token);
}

void on_send_failure(void *context, MQTTAsync_failureData5 *response) {
    dprintf(LOGLEVEL_ERR,
            "Sending message failed for token %d, error code: %d\n",
            response->token,
            response->code);
}

/* MQTT settings */
const char *MQTT_ADDRESS = "tcp://192.168.1.80:1883";
const char *MQTT_TOPIC = "winner/powerreader/results";
const int MQTT_QOS_DEFAULT = 0;
const char *MQTT_CLIENT_ID = "Starterkit";
const int MQTT_KEEPALIVE_S = 20;

// whether the topic has already been sent to the server, so we can use an alias istead
bool topicSent = false;

/**
 * @brief Initializes the MQTT client and connects it to the broker.
 *
 * @retval The initialized MQTT client
 */
MQTTAsync MQTT_init_and_connect() {
    MQTTAsync client;
    MQTTAsync_createOptions createOpts = MQTTAsync_createOptions_initializer5;
    createOpts.deleteOldestMessages = 1;
    createOpts.restoreMessages = 0;
    // No persistence should be fine. Messages get out of date immediately and
    // we can always get a reference value from the module later.
    MQTTAsync_createWithOptions(&client, MQTT_ADDRESS, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    MQTTAsync_setCallbacks(client, NULL, on_connection_lost, on_message_arrived, NULL);

    MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer5;
    connOpts.context = client;
    connOpts.keepAliveInterval = 20;
    connOpts.automaticReconnect = 1;
    connOpts.onSuccess5 = on_connect_success;
    connOpts.onFailure5 = on_connect_failure;

    int connectResult;
    if ((connectResult = MQTTAsync_connect(client, &connOpts)) != MQTTASYNC_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Failed to start connect, return code %d\n", connectResult);
    }

    return client;
}

/**
 * @brief Disconnects and destroys the MQTT client
 *
 * @param[in] client The MQTT client
 */
void MQTT_disconnect_and_destroy(MQTTAsync client) {
    MQTTAsync_disconnectOptions discOpts = MQTTAsync_disconnectOptions_initializer5;
    discOpts.onSuccess5 = on_disconnect;
    discOpts.onFailure5 = on_disconnect_failure;

    if (MQTTAsync_isConnected(client)) {
        int disconnectResult;
        if ((disconnectResult = MQTTAsync_disconnect(client, &discOpts)) != MQTTASYNC_SUCCESS) {
            dprintf(LOGLEVEL_ERR, "Failed to start disconnect, return code %d\n", disconnectResult);
        } else {
            // doing a proper check would probably be cleaner, but this works just fine
            usleep(20000);
        }
    }

    MQTTAsync_destroy(&client);
}

/**
 * @brief Turns a ResultSet into a human-readable string to be sent out via MQTT.
 *
 * @param[in] results A pointer to the completed ResultSet instance
 * @retval The resulting string allocated on the heap
 */
char *get_MQTT_message_string(ResultSet *results) {
    char resultBuf[4096];
    char lineBuf[128];
    memset(resultBuf, 0, sizeof(resultBuf));
    memset(lineBuf, 0, sizeof(lineBuf));

    sprintf(resultBuf,
            "Module Index: %u\nTimestamp: %ld.%ld\n",
            results->moduleIndex,
            results->timestamp.tv_sec,
            (long)(results->timestamp.tv_nsec / 1E6));
    for (size_t i = 0; i < results->size; i++) {
        sprintf(lineBuf,
                "%s: %.2f %s\n",
                results->descriptions[i]->description,
                results->values[i],
                results->descriptions[i]->unit);
        strcat(resultBuf, lineBuf);
    }
    char *result = malloc(strlen(resultBuf) + 1);
    if (result == NULL) {
        return NULL;
    }
    strcpy(result, resultBuf);
    return result;
}

/**
 * @brief Packs a given ResultSet into a ResultSetMsg Protocol buffer
 *
 * This function creates packed protocol buffer from a ResultSet, ready to be sent via MQTT.
 * It follows the definition of ResultSetMsg in protobuf/result_set.proto which currently
 * contains fields for the voltage, effective power and reactive power, and assumes we receive
 * three measurements of each one. Other measurements are ignored. If there are additional
 * measurements to be taken, the .proto file and this function need to be updated in tandem
 * (this is the price to pay for the small memory footprint).
 * 
 * @param[in] results A pointer to the completed ResultSet instance
 * @param[out] size The size of the resulting message
 * @retval A pointer to the buffer containing the packed message
 */
void *get_MQTT_protobuf_message(ResultSet *results, size_t *size) {
    ResultSetMsg msg = RESULT_SET_MSG__INIT;
    void *buf;
    uint32_t voltage[3];
    int32_t effective_power[3];
    int32_t reactive_power[3];

    msg.index = results->moduleIndex;
    double usecs = round(results->timestamp.tv_nsec / 1E6) / 1000;
    msg.timestamp = results->timestamp.tv_sec + usecs;

    // Fill the results. We trust that the values for each phase are in correct order
    // and there are no duplicate entries, otherwise this would need to be a lot more complicated
    // Results are upscaled by a factor of 1000 in order to transmit them as integer values, but
    // perhaps there could be a cleaner way to do it in the future by getting rid of the intermediary
    // double altogether
    size_t v_i = 0, ep_i = 0, rp_i = 0;
    for (size_t i = 0; i <= results->size; i++) {
        MET_ID_AC id = results->descriptions[i]->metID;
        if (id == VOLTAGE_RMS_L1N || id == VOLTAGE_RMS_L2N || id == VOLTAGE_RMS_L3N) {
            voltage[v_i] = (uint32_t)(results->values[i] * 1000);
            v_i++;
        }
        else if (id == POWER_EFFECTIVE_L1 || id == POWER_EFFECTIVE_L2 || id == POWER_EFFECTIVE_L3) {
            effective_power[ep_i] = (int32_t)(results->values[i] * 1000);
            ep_i++;
        }
        else if (id == POWER_REACTIVE_L1 || id == POWER_REACTIVE_L2 || id == POWER_REACTIVE_L3) {
            reactive_power[rp_i] = (int32_t)(results->values[i] * 1000);
            rp_i++;
        }
    }

    msg.n_voltage = 3;
    msg.n_effective_power = 3;
    msg.n_reactive_power = 3;
    msg.voltage = voltage;
    msg.effective_power = effective_power;
    msg.reactive_power = reactive_power;

    *size = result_set_msg__get_packed_size(&msg);
    buf = malloc(*size);
    if (buf == NULL) {
        return NULL;
    }

    result_set_msg__pack(&msg, buf);
    return buf;
}

/**
 * @brief Sends a ResultSet using MQTT 5
 *
 * @param[in] client The properly initialized MQTT client
 * @param[in] results A pointer to the comleted ResultSet
 * @retval ERROR_SUCCESS on success, another error code otherwise
 */
ErrorCode send_MQTT5_message(MQTTAsync client, ResultSet *results) {
    MQTTAsync_responseOptions responseOpts = MQTTAsync_responseOptions_initializer;
    responseOpts.onSuccess5 = on_send;
    responseOpts.onFailure5 = on_send_failure;
    responseOpts.context = client;

    // Paho will handle the deallocation of msg for us, so we don't have to worry about it
    size_t msgLength;
    void *msg = get_MQTT_protobuf_message(results, &msgLength);
    if (msg == NULL) {
        dprintf(LOGLEVEL_ERR, "Failed to create the MQTT message\n");
        return -ERROR_MQTT_MSG_CREATION_FAILED;
    }
    MQTTProperties messageProps = MQTTProperties_initializer;
    MQTTProperty aliasProp = {
        .identifier = MQTTPROPERTY_CODE_TOPIC_ALIAS,
        .value = { .integer2 = 1 }
    };
    messageProps.array = &aliasProp;
    messageProps.length = sizeof(aliasProp);
    messageProps.count = 1;
    messageProps.max_count = 1;

    MQTTAsync_message message = MQTTAsync_message_initializer;
    message.payload = msg;
    message.payloadlen = msgLength;
    message.qos = MQTT_QOS_DEFAULT;
    message.properties = messageProps;

    const char *topic = topicSent ? "" : MQTT_TOPIC;

    int pubResult;
    if ((pubResult = MQTTAsync_sendMessage(client, topic, &message, &responseOpts)) != MQTTASYNC_SUCCESS) {
        dprintf(LOGLEVEL_ERR, "Failed to start sendMessage, return code %d\n", pubResult);
        return -ERROR_MQTT_MSG_SEND_FAILED;
    } else {
        topicSent = true;
    }

    return ERROR_SUCCESS;
}

#endif
