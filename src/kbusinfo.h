/******************************************************************************/
//
// Created by getkbusinfo at 2020-09-14_15:36:00
//
/******************************************************************************/

#ifndef KBUSINFO_H
#define KBUSINFO_H

// In this array you will find all infos about the scanned kbus
// Position, ModulType, BitOffsetOut, BitSizeOut, BitOffsetIn, BitSizeIn, Channels, PiFormat

#define NROFKBUSMODULES 3
#define NROFINFOROWS 8
//extern const unsigned short kbusSimpleInfo[NROFKBUSMODULES][NROFINFOROWS];

// The typedef struct for the scanned kbusInput
typedef struct __attribute__((packed))
{
	unsigned char p3t495c1[24];	// Position 3, Typ 750-495, Channel 1
	unsigned int p1t4XXc1:1;	// Position 1, Typ 750-4XX, Channel 1
	unsigned int p1t4XXc2:1;	// Position 1, Typ 750-4XX, Channel 2
} tKbusInput;

// The typedef struct for the scanned kbusOutput
typedef struct __attribute__((packed))
{
	unsigned char p3t495c1[24];	// Position 3, Typ 750-495, Channel 1
	unsigned int p2t5XXc1:1;	// Position 2, Typ 750-5XX, Channel 1
	unsigned int p2t5XXc2:1;	// Position 2, Typ 750-5XX, Channel 2
} tKbusOutput;

// The byte offset of the bit input field in tKbusInput
#define BYTEOFFSETINPUTBITFIELD 24

// The byte offset of the bit output field in tKbusOutput
#define BYTEOFFSETOUTPUTBITFIELD 24

// The structs and the pointer to byte- and bitfields
//extern tKbusInput kbusInputData;
//extern tKbusOutput kbusOutputData;

//extern uint8_t * pKbusInputBitData;
//extern uint8_t * pKbusInputData;

//extern uint8_t * pKbusOutputBitData;
//extern uint8_t * pKbusOutputData;

#endif // KBUSINFO_H
