#include <stdio.h>
#include <stdlib.h>
#include "./bus.h"

BUS InitBUS(uint8_t *program, uint32_t sizeOfProgram){

    //init devices

    //init DRAM
    DRAM dram = InitDRAM(program, sizeOfProgram);

    BUS bus = {dram};
    printf("> System bus initalized\n");
    return bus;
}

void DeInitBUS(BUS bus){

    //deinit devices (freeing memory)
    DeinitDRAM(bus.dram);
    printf("> System bus deinitalized\n");
}

uint64_t BusLoad(BUS bus, uint64_t address, uint8_t size){
    //akin to dram, size can either be 8, 16, 32 or 64 and refers to the size in bits of the value being retireved or stored

    if(address <= DRAM_BASE){
        //then the address refers to some place in DRAM
         if(bus.dram.dram == NULL){
            printf("[Err]: dram pointer in dram struct in the bus struct is pointing to NULL.\n[Err cont.]:Bus LOAD fail \n[Err cont.]: Check bus.c, then dram.c for potential bug.\n");
            return;
        }

        printf("> Bus LOAD command executed succesfully\n");
        DRAMload(bus.dram, address, size);
    
    }
}

void BusStore(BUS bus, int64_t address, uint8_t size, uint64_t val){
    if(address <= DRAM_BASE){
        //then the address refers to some place in DRAM
         if(bus.dram.dram == NULL){
            printf("[Err]: dram pointer in dram struct in the bus struct is pointing to NULL.\n[Err cont.]:Bus STORE fail \n[Err cont.]: Check bus.c, then dram.c for potential bug.\n");
            return;
        }

        printf("> Bus STORE command executed succesfully\n");
        DRAMstore(bus.dram, address, size, val);
    
    }
}