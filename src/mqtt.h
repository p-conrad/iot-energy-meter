#ifndef MQTT_H
#define MQTT_H

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "MQTTAsync.h"
#include "unit_description.h"
#include "utils.h"

/**
 * @brief A structure representing a complete set of results. It is intended to be created from the finished ResultSet
 *        as an independent entity, to be exclusively handled by the asynchronous MQTT routines while the main loop
 *        keeps collecting new measurements.
 */
typedef struct CompleteResultSet {
	const UnitDescription **descriptions;
	const size_t size;
	const double *values;
	const struct timespec timestamp;
} CompleteResultSet;

/**
 * @brief Allocates and returns a CompleteResult set from a given ResultSet.
 *
 * This is supposed to be called from the main loop only after the ResultSet has been finished, so no checks for
 * completeness are done here.
 *
 * @param[in] results A pointer to the finished ResultSet instance
 * @retval A pointer to the allocated CompleteResultSet, or NULL on allocation failure
 */
CompleteResultSet *allocate_crs(const ResultSet *results) {
	CompleteResultSet *crs = malloc(sizeof(CompleteResultSet));
	if (crs == NULL) {
		return NULL;
	}

	crs->descriptions = results->descriptions;
	*(size_t *)&crs->size = results->size;

	crs->values = malloc(sizeof(double) * crs->size);
	if (crs->values == NULL) {
		free(crs);
		return NULL;
	}
	memcpy((void *)crs->values, results->values, sizeof(double) * crs->size);
	clock_gettime(CLOCK_TAI, (struct timespec *)&crs->timestamp);

	return crs;
}

/* MQTT callbacks */
void on_connect_success(void *context, MQTTAsync_successData *response) {
    dprintf(LOGLEVEL_INFO, "Connection to the MQTT broker successful.\n");
}

void on_connect_failure(void *context, MQTTAsync_failureData *response) {
    dprintf(LOGLEVEL_ERR,
            "Connection to the MQTT broker failed, response code: %d\n",
            response->code);
}

void on_disconnect(void *context, MQTTAsync_successData *response) {
    dprintf(LOGLEVEL_INFO, "Successfully disconnected.\n");
}

void on_disconnect_failure(void *context, MQTTAsync_failureData *response) {
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

void on_send(void *context, MQTTAsync_successData *response) {
    dprintf(LOGLEVEL_DEBUG,
            "Message with token value %d delivery confirmed\n",
            response->token);
}

void on_send_failure(void *context, MQTTAsync_failureData *response) {
    dprintf(LOGLEVEL_ERR,
            "Message send failed token %d error code %d\n",
            response->token,
            response->code);
}

/* MQTT settings */
const char *MQTT_ADDRESS = "tcp://192.168.1.80:1883";
const char *MQTT_TOPIC = "winner/powerreader/results";
const int MQTT_QOS_DEFAULT = 0;
const char *MQTT_CLIENT_ID = "Starterkit";
const int MQTT_KEEPALIVE_S = 20;

/**
 * @brief Initializes the MQTT client and connects it to the broker.
 *
 * @retval The initialized MQTT client
 */
MQTTAsync MQTT_init_and_connect() {
    MQTTAsync client;
    // No persistence should be fine. Messages get out of date immediately and
    // we can always get a reference value from the module later.
    MQTTAsync_create(&client, MQTT_ADDRESS, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, NULL, on_connection_lost, on_message_arrived, NULL);

    MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer;
    connOpts.context = client;
    connOpts.keepAliveInterval = 20;
    connOpts.cleansession = 1;
    connOpts.automaticReconnect = 1;
    connOpts.onSuccess = on_connect_success;
    connOpts.onFailure = on_connect_failure;

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
    MQTTAsync_disconnectOptions discOpts = MQTTAsync_disconnectOptions_initializer;
    discOpts.onSuccess = on_disconnect;
    discOpts.onFailure = on_disconnect_failure;

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

#endif
