#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Definition of possible error codes
 */
typedef enum ErrorCode {
    ERROR_SUCCESS = 0,
    ERROR_KBUS_NOT_FOUND,
    ERROR_KBUS_OPEN_FAILED,
    ERROR_STATE_CHANGE_FAILED,
    ERROR_DEVICE_SPECIFIC_FUNCTION_FAILED,
    ERROR_LIBPACKBUS_PUSH_FAILED,
    ERROR_ALLOCATION_FAILED,
    ERROR_KBUSINFO_CREATE_FAILED,
    ERROR_KBUSINFO_STATUS_FAILED,
    ERROR_KBUSINFO_TERMINAL_INFO_FAILED,
    ERROR_KBUSINFO_TERMINAL_LIST_FAILED,
    ERROR_NO_MODULES,
    ERROR_MQTT_MSG_CREATION_FAILED,
    ERROR_MQTT_MSG_SEND_FAILED,
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

/**
 * @brief Calls a function and exits the program in case of an error.
 *
 * Most of the calls to the ADI/DAL interface can potentially result in an error,
 * the handling of which always follows the same pattern of first closing the device,
 * then exiting the ADI and closing the program. This macro helps removing some
 * of the redundancy.
 *
 * @param[in] call A function call returning an ErrorEcode
 */
#define exit_on_error(call) do {            \
    ErrorCode result = call;                \
    if (result != ERROR_SUCCESS) {          \
        adi->CloseDevice(kbusDeviceId);     \
        adi->Exit();                        \
        return result;                      \
    }                                       \
} while (0)

#endif
