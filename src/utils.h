#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Definition of possible error codes
 */
typedef enum ErrorCode {
    ERROR_LIBPACKBUS_PUSH_FAILED = -5,
    ERROR_DEVICE_SPECIFIC_FUNCTION_FAILED,
    ERROR_STATE_CHANGE_FAILED,
    ERROR_KBUS_OPEN_FAILED,
    ERROR_KBUS_NOT_FOUND,
    ERROR_SUCCESS
} ErrorCode;

/**
 * @brief Definition of verbosity levels
 */
typedef enum Loglevel {
    LOGLEVEL_EMERG,
    LOGLEVEL_ALERT,
    LOGLEVEL_CRIT,
    LOGLEVEL_ERR,
    LOGLEVEL_WARNING,
    LOGLEVEL_NOTICE,
    LOGLEVEL_INFO,
    LOGLEVEL_DEBUG
} Loglevel;

/**
 * @brief The log level to use.
 */
extern Loglevel loglevel;

/**
 * @brief Definition of helper function. It allows to print out debug information at several verbosity levels.
 *
 * @param[in] printlevel At given minimum level it will be printed
 * @param[in] format Like printf format
 * @param[in] ... VA_ARGS like printf
 */
#define dprintf(printlevel, format, ...) do {       \
        if (loglevel >= printlevel)                   \
            fprintf(stderr, format, ##__VA_ARGS__); \
} while(0)

#endif
