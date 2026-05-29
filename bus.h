#ifndef BUS_H
#define BUS_H

#include "./dram.h"

//constants
#define DRAM_BASE 0x80000000ULL //ULL means treat this number as an unsigned long long integer
//for our purposes ULL suffix shouldn't matter, but this explictly makes the compiler interpet this as an unsigned long long integer

//type declarations
typedef struct Bus{
    DRAM dram;
} BUS;

//prototypes
BUS InitBUS(uint8_t *program, uint32_t sizeOfProgram);
void DeInitBUS(BUS bus);
uint64_t BusLoad(BUS bus, uint64_t address, uint8_t size);
void BusStore(BUS bus, int64_t address, uint8_t size, uint64_t val);

#endif