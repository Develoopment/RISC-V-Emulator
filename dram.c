#include <stdio.h>
#include <stdlib.h>

#include "./dram.h"

//const unsigned int DRAM_SIZE = 1024 * 1024 * 128; //(1024 bytes in a kibibytes) * (1024 kbibytes in a mibibyte) * 128 MiB
const unsigned int DRAM_SIZE = 1024 * 1024 * 128; //(1024 bytes in a kibibytes) * (1024 kbibytes in a mibibyte) * 128 MiB

//our DRAM has a size of 128 MiB

//given the bytes from the .elf file, place it at the beginning of DRAM
//this function will (check to make sure this is true) be called when initalizing DRAM by the main program
DRAM InitDRAM(uint8_t *program, uint32_t sizeOfProgram) { //takes in the .elf file here, read and passed in when DRAM will be initalized
    DRAM dramObj = {NULL};
    
    //allocate space
    uint8_t *dramPointer = calloc(DRAM_SIZE, sizeof(uint8_t));
    if(dramPointer == NULL){
        printf("[Err]: No space on the heap. DRAM could not be initalized\n");
        return dramObj;
    }

    dramObj.dram = dramPointer;
    printf("> DRAM INITALIZED WITH %u bytes OF MEMORY ON THE HEAP\n", DRAM_SIZE);

    //read the program from the elf file and splice it into DRAM
    //!WE ARE AGREEING TO OURSELVES THAT THE ELF BINARY AND HENCE THE PROGRAM WILL START AT X0000 SO I AM LOADING THE BINARY THERE
    //copying over binary into DRAM
    //ERROR CHECKING
    if(sizeOfProgram > DRAM_SIZE){
        printf("[Err]: Program larger than 128MiB, program is larger than DRAM capacity\n");
        free(dramPointer);
        dramObj.dram = NULL;
        return dramObj;
    }else if(sizeOfProgram == 0){
        printf("[WARNING]: Program has a size of 0. Program is empty\n");
    }

    for(uint32_t i = 0; i < sizeOfProgram; i++){
        dramObj.dram[i] = program[i];
    }

    printf("> GIVEN BINARY SUCCESSFULLY COPIED INTO DRAM\n");
    return dramObj;
}

void DeinitDRAM(DRAM dramObj){
    free(dramObj.dram);
    printf("> DRAM SUCCESSFULLY Uninitialized\n");
}

//address is the size is where in the DRAM the access is happening, and size can either be 8, 16, 32, or 64

//the int size of address needs to be big enough to access each byte of the (1024*1024*128 bytes) which needs atleast 27 bits to represent this, but to be safe we are using 64 bits
uint64_t DRAMload(DRAM dramObj, uint64_t address, uint8_t size){
    
    uint8_t iteratorSize = size/8;
    uint64_t returnVal = 0;

    //error checks
    if(size > 64 || (size%8 != 0)){
        printf("[Err]: invalid size for load command. Load not successful\n");
        return 0;
    }
    //boundary check
    if(address+iteratorSize > DRAM_SIZE){
        printf("[Err]: attempting to access memory greater than DRAM capacity. Load could not be completed.\n");
        return 0;
    }

    for(uint8_t i = 0; i < iteratorSize; i++){
        returnVal |= ((uint64_t)dramObj.dram[address+i] << (i*8));
    }

    //!remember that items in memory are stored in little endian form
    printf("> LOAD COMMAND FOR %d bytes AT MEM ADDRESSS: %lx SUCCESSFUL\n", size, address);
    return returnVal;
}

void DRAMstore(DRAM dramObj, uint64_t address, uint8_t size, uint64_t val){
    //!remember that items in memory are stored in little endian form
    uint8_t iteratorSize = size/8;

    if(size > 64 || (size%8 != 0)){
        printf("[Err]: Invalid size of value. Store could not be completed\n");
        return;
    }
    if(address+iteratorSize > DRAM_SIZE){
        printf("[Err]: attempting to access memory greater than DRAM capacity. Store could not be completed.\n");
    }


    for(uint8_t i = 0; i < iteratorSize; i++){
        //get LSB
        uint8_t storeVal = (uint8_t)(val >> (i*8));
        //store at lower address
        dramObj.dram[address+i] = storeVal;
    }
    

    printf("> STORE COMMAND FOR VALUE %lx AT MEM ADDRESSS: %lx SUCCESSFUL\n", val, address);
}