#ifndef KBUSINFO_H
#define KBUSINFO_H

#include "process_image.h"

/**
 * @brief A structure representing the process input of the current system in use.
 */
typedef struct __attribute__((packed))
{
    Type495ProcessInput t495Input;     // Position 3, Typ 750-495, Channel 1
    unsigned int p1t4XXc1:1;           // Position 1, Typ 750-4XX, Channel 1
    unsigned int p1t4XXc2:1;           // Position 1, Typ 750-4XX, Channel 2
} tKbusInput;

/**
 * @brief A structure representing the process output of the current system in use.
 */
typedef struct __attribute__((packed))
{
    Type495ProcessOutput t495Output;   // Position 3, Typ 750-495, Channel 1
    unsigned int p2t5XXc1:1;           // Position 2, Typ 750-5XX, Channel 1
    unsigned int p2t5XXc2:1;           // Position 2, Typ 750-5XX, Channel 2
} tKbusOutput;

// The byte offset of the bit input field in tKbusInput
#define BYTEOFFSETINPUTBITFIELD 24

// The byte offset of the bit output field in tKbusOutput
#define BYTEOFFSETOUTPUTBITFIELD 24

#endif // KBUSINFO_H
