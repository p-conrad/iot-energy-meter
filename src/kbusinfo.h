#ifndef KBUSINFO_H
#define KBUSINFO_H

#include "process_image.h"

// The typedef struct for the scanned kbusInput
typedef struct __attribute__((packed))
{
    type495processInput t495Input;     // Position 3, Typ 750-495, Channel 1
    unsigned int p1t4XXc1:1;           // Position 1, Typ 750-4XX, Channel 1
    unsigned int p1t4XXc2:1;           // Position 1, Typ 750-4XX, Channel 2
} tKbusInput;

// The typedef struct for the scanned kbusOutput
typedef struct __attribute__((packed))
{
    type495processOutput t495Output;   // Position 3, Typ 750-495, Channel 1
    unsigned int p2t5XXc1:1;           // Position 2, Typ 750-5XX, Channel 1
    unsigned int p2t5XXc2:1;           // Position 2, Typ 750-5XX, Channel 2
} tKbusOutput;

// The byte offset of the bit input field in tKbusInput
#define BYTEOFFSETINPUTBITFIELD 24

// The byte offset of the bit output field in tKbusOutput
#define BYTEOFFSETOUTPUTBITFIELD 24

#endif // KBUSINFO_H
