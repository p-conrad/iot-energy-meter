#ifndef MQTT_H
#define MQTT_H

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "unit_description.h"

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
 * @brief Allocates and returns a ComleteResult set from a given ResultSet.
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

#endif
