//this is going to be the actual CPU
#include <stdio.h>
#include <stdint.h>
#include "./bus.h"
#include <stdlib.h>
#include <string.h>
#include <elf.h>

//this struct holds the STATE of the CPU
//1. State declaration
typedef struct{
    uint64_t regs[32];  //initiating all to 0 for testing purposes
    uint64_t PC; //starting Program Counter at 0x0000

    //replaced memory member with bus member
    BUS bus;
}cpu;

//helper functions
void dump_regs(cpu *CPU){
    uint32_t sizeOfArray = sizeof(CPU->regs) / sizeof(CPU->regs[0]);
    printf("=================== REGISTERS DUMP ====================\n\n");
    for(uint32_t i = 0; i < sizeOfArray; i++){
        printf("Reg %d: %lx\n", i, CPU->regs[i]);
    }
}

int main(){

    //2.Parse and read .elf file
    FILE *f = fopen("test.elf", "rb"); //open the .elf file and read as bytes
    fseek(f, 0, SEEK_END); //set cursor to the end of the file
    long fileSize = ftell(f); //ftell gets the current position of the cursor
    rewind(f); //puts the cursor back at the beginning of the file

    //reading the elf file into a buffer allows us to access it using pointers (or indexes in our case)
    uint8_t *elffile = malloc(fileSize);
    if(elffile == NULL){
        printf("[Err]: no space to read elffile on the heap.\n");
        return 0;
    }
    fread(elffile, 1, fileSize, f); //read using 1 byte from the file into the buffer
    fclose(f);

    Elf64_Ehdr *header = (Elf64_Ehdr *)elffile; //WHAT DOES THIS DO EXACTLY?

    //verify if the file we got is actually an elf file
    //!elf is a type of binary file that has headers containing metadata
    //one such header is e_ident which has bytes that identify this file as a valid .elf file
    if(header->e_ident[0] != 0x7f || header->e_ident[1] != 'E' || header->e_ident[2] != 'L' ||header->e_ident[3] != 'F'){
        printf("[Err]: Not a valid .elf file\n");
        return 0;
    }

    //!program headers describe where different segements of the .elf file go, each segment is type Elf64_Phdr
    //!the program header table starts at buffer ehdr->e_phoff where phoff is a header and e_phnum is the header that tells you howm any segments there are
    uint8_t *actualProgramBytes = calloc(DRAM_SIZE, 1);
    for(int i = 0; i < header->e_phnum; i++){
        Elf64_Phdr *phdr = (Elf64_Phdr *)(elffile + header->e_phoff + i * header->e_phentsize);
    
        //if the phdr has a tpye of PT_LOAD then that is what goes in our DRAM, other types load elsewhere which we will see later prolly
        if(phdr->p_type == PT_LOAD){
            //copy p_filesz bytes from file offset p_offset into program at the segments address
            memcpy(actualProgramBytes + phdr->p_vaddr, elffile + phdr->p_offset, phdr->p_filesz);
        }
    
    }

    cpu CPU = {0};
    CPU.bus = InitBUS(actualProgramBytes, DRAM_SIZE);
    free(actualProgramBytes);

    //3. Populating special registers
    //according to the RISC V ISA reg 2 is the SP and reg 0 is hardwired to be all 0s
    CPU.regs[0] = 0;
    CPU.regs[2] = 256; //letting it point to the last memory cell, since stack grows downward, any nonzero value here would work

    //------TESTING PARSING FROM A CUSTOM ASM FILE------//
    CPU.regs[5] = 3;
    CPU.regs[6] = 5;

    //4. Instruction Cycle (managed by the FSM in the datapath)
    //the instruction cycle is composed of: fetch instruction, decode instruction, eval address, fetch operands, execute, store value
    while(CPU.PC < 10){//will need to change this

        //FETCH INSTRUCTION
        //changed fetch instruction to load it from the parsed elf file
        uint32_t instruction = (uint32_t)(BusLoad(CPU.bus, CPU.PC, 32));
        
        CPU.PC += 4; //point to next instruction
            //RISCV instrucitons are 32 bit because if there are 32 registers then we need 5 bits to represent a register 0-31.
            //since instrucitons looke like operation [dest. reg] [reg 1] [reg 2] thats 5+5+5 = 15 and 1 more bit isn't enough for ther operation so we can't use 16 bits ==> next smallest is 32 bits
        
        //DECODE INSTRUCTION and make material based on opcode
        uint8_t opcode = instruction & (0b1111111);

        //! Opcodes in RISC V specify what format type the instruction is in, other bits then specify what operation is actually being performed (i.e. ADD, AND, etc)
        switch (opcode)
        {
        case 0x33:{ //R Types

            //get the 3 bit differentiator
            uint8_t operation = (instruction & (0b111 << 12)) >> 12;

            switch (operation){

                case 0:{ //ADD
                    uint8_t rd = (instruction & (0b11111 << 7)) >> 7;
                    uint8_t rs1 = (instruction & (0b11111 << 15)) >> 15;
                    uint8_t rs2 = (instruction & (0b11111 << 20)) >> 20;

                    CPU.regs[rd] = CPU.regs[rs1] + CPU.regs[rs2]; //? OVERFLOW CHECKING
                    printf("> ADD executed\n");
                    break;
                }

                default:{
                    printf("[Err]: R-type operation NOT IMPLEMENTED YET\n");
                    break;
                }
            }
            
            break;
        }

        case 0x13:{ //I type
            
            break; 
        }
        
        default:{
            printf("[Err]: I-type operation NOT IMPLEMENTED YET\n");
            break;
        }
        }


        //EXECUTE (+ EVAL ADRESSES)
       
    


    }

    DeInitBUS(CPU.bus);
    dump_regs(&CPU);

    return 0;

}
