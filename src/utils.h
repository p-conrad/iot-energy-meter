#include <stdint.h>

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

/**
 * @brief Reads a uint32 from a buffer at a given offset where bytes are in ascending order.
 *
 * @param[in] buf The buffer to read from
 * @param[in] offset The offset in buf to read from
 * @retval The uint32_t read from the buffer
 */
uint32_t read_uint32(uint8_t* buf, uint32_t offset) {
    uint32_t result = 0;
    result |= buf[offset + 3] << 24;
    result |= buf[offset + 2] << 16;
    result |= buf[offset + 1] << 8;
    result |= buf[offset];
    return result;
}

/**
 * @brief Reads a int32 from a buffer at a given offset where bytes are in ascending order.
 *
 * @param[in] buf The buffer to read from
 * @param[in] offset The offset in buf to read from
 * @retval The uint32_t read from the buffer
 */
int32_t read_int32(uint8_t* buf, uint32_t offset) {
	return read_uint32(buf, offset);
}
