#ifndef DRAM_H
#define DRAM_H

#include <stdint.h>

//Constants
extern const unsigned int DRAM_SIZE; //(1024 bytes in a kibibytes) * (1024 kbibytes in a mibibyte) * 128 MiB

//Type Declarations
//our DRAM has a size of 128 MiB
typedef struct DRAM{
    uint8_t *dram; //program will be stored on the heap
} DRAM;

//Function Prototypes
DRAM InitDRAM(uint8_t *program, uint32_t sizeOfProgram);
void DeinitDRAM(DRAM dramPointer);
uint64_t load(DRAM *dramPointer, uint64_t address, uint8_t size);
void store(DRAM *dramPointer, uint64_t address, uint8_t size, uint64_t val);

#endif