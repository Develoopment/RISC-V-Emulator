//this is going to be the actual CPU
#include <stdio.h>
#include <stdint.h>
#include "./bus.h"
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#define SIE 0x104
#define MIE 0x304
#define MIDELEG 0x303

//this struct holds the STATE of the CPU
//1. State declaration
typedef struct{
    uint64_t regs[32];  //initiating all to 0 for testing purposes
    uint64_t PC; //starting Program Counter at 0x0000

    //replaced memory member with bus member
    BUS bus;

    uint64_t csrs[4096]; //csrs are their own register file and manage switching between supervisor and user mode
}cpu;

//helper functions
uint64_t load_csr(cpu *CPU, uint16_t csr_address){
    if(csr_address == SIE){
        return CPU->csrs[MIE] & CPU->csrs[MIDELEG]; //if the csr we want to get is SIE, we need to get the MIE and mask it with MIDELEG which is another register that tells the program what bits the supervisor mode can see
    }else{
        return CPU->csrs[csr_address];
    }
}

void store_csr(cpu *CPU, uint16_t csr_address, uint64_t value){
    if(csr_address == SIE){
        CPU->csrs[MIE] = (CPU->csrs[MIE] & ~CPU->csrs[MIDELEG]) | (value & CPU->csrs[MIDELEG]);
        //this bitwise operation clears out bits that can be enabled by supervisor mode and then the other part lets only those bits to be written to the MIE register
    }else{
        CPU->csrs[csr_address] = value; 
    }
}

void dump_regs(cpu *CPU){
    uint32_t sizeOfArray = sizeof(CPU->regs) / sizeof(CPU->regs[0]);
    printf("=================== REGISTERS DUMP ====================\n\n");
    for(uint32_t i = 0; i < sizeOfArray; i++){
        printf("Reg %d: %lx (%d)\n", i, CPU->regs[i], CPU->regs[i]);
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
    uint64_t prevPC = 0; //REMOVE AFTER IMPLEMENTING ECALL

    //to show regs state before the program
    printf("***Initial State of Registers:***\n");
    dump_regs(&CPU);

    //4. Instruction Cycle (managed by the FSM in the datapath)
    //the instruction cycle is composed of: fetch instruction, decode instruction, eval address, fetch operands, execute, store value
    while(1){//will need to change this

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

        case 0x03:{//LD RV64I
            
            uint8_t operation = (instruction & ((uint64_t)0b111 << 12)) >> 12;

            switch (operation)
            {
            case 0x03:
                
                uint8_t rd = (instruction & ((uint64_t)0b11111 << 7)) >> 7;
                uint8_t rs1 = (instruction & ((uint64_t)0b11111 << 15)) >> 15;
                int64_t offset = (int64_t)((int32_t)instruction >> 20); 
                //cant do (int64_t)instruction >> 20; because instruction is unsigned 32 bit value, any casting to a higher bit value will zero extend it including int64_t will zero extend it. We must first cast it into int32_t to ensure negative values are extended correctly.
                //this is a int64_t type. signed because offset can point to memory before or after the address in rs1. 64 bit because the value in rs1 is 64 bit and because we need to sign extend the signed immediate value to 64 bits before adding it to the base.
                
                CPU.regs[rd] = BusLoad(CPU.bus, CPU.regs[rs1] + offset, 64);
                printf("> Loaded 64 bit value from DRAM into reg: %d\n", rd);

                break;
            
            default:
                printf("[Err]: RV64I Load type instruction? not implemented yet\n");
                break;
            }

            break;
        }
        
        case 0x23:{//SD RV64I
            uint8_t operation = (instruction & ((uint64_t)0b111 << 12)) >> 12;

            switch (operation)
            {
            case 0x03:
                
                uint8_t srcreg = (instruction & ((uint64_t)0b11111 << 20)) >> 20;
                uint8_t basereg = (instruction & ((uint64_t)0b11111 << 15)) >> 15;
                int64_t offset =  (((int32_t)instruction >> 20) & ~0x1FLL) | ((instruction >> 7) & 0x1F);
                
                BusStore(CPU.bus, CPU.regs[basereg] + offset, 64, CPU.regs[srcreg]);
                printf("> Stored 64 bit value into DRAM from reg: %d\n", srcreg);

                break;
            
            default:
                printf("[Err]: RV64I Store type instruction? not implemented yet\n");
                break;
            }

            break;
        }

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

                case 0x07:{ //AND
                    uint8_t rd = (instruction & (0b11111 << 7)) >> 7;
                    uint8_t rs1 = (instruction & (0b11111 << 15)) >> 15;
                    uint8_t rs2 = (instruction & (0b11111 << 20)) >> 20;

                    CPU.regs[rd] = CPU.regs[rs1] & CPU.regs[rs2];
                    printf("> AND executed\n");
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

            //get the 3 bit differentiator
            uint8_t operation = (instruction & (0b111 << 12)) >> 12;

            switch (operation){

                case 0x00:{ //ADDI
                    uint8_t rd = (instruction & (0b11111 << 7)) >> 7;
                    uint8_t rs1 = (instruction & (0b11111 << 15)) >> 15;
                    //immediate is stored in bits [31:20], sign-extended to 64 bits
                    int64_t imm = (int64_t)((int32_t)instruction >> 20);

                    CPU.regs[rd] = CPU.regs[rs1] + imm;
                    printf("> ADDI executed\n");
                    break;
                }

                default:{
                    printf("[Err]: I-type operation NOT IMPLEMENTED YET\n");
                    break;
                }
            }

            break;
        }

        case 0x63:{//B type (branches)

            uint8_t operation = (uint8_t)((instruction & ((uint32_t)(0b111) << 12)) >> 12);
            uint8_t rs1 = (uint8_t)((instruction & ((uint32_t)(0b11111) << 15)) >> 15);
            uint8_t rs2 = (uint8_t)((instruction & ((uint32_t)(0b11111) << 20)) >> 20);

            //the 12 bit offset value is stored across bits in this encoding so lets reassemble that
            //imm[0] is always 0 since the offsets are implied to be 2 byte aligned (why? I dunno? quirk of hardware ig)
            int64_t offset = ((int64_t)((int32_t)instruction >> 31) << 12)  // imm[12], sign-extended
               | (((instruction >>  7) & 0x1)  << 11)           // imm[11]
               | (((instruction >> 25) & 0x3F) <<  5)           // imm[10:5]
               | (((instruction >>  8) & 0xF)  <<  1);          // imm[4:1]


            switch (operation)
            {
            case 0x01:{ //BNE branch if not equal
                if(CPU.regs[rs1] != CPU.regs[rs2]){
                    //branch
                    CPU.PC = (CPU.PC - 4) + offset; //since earlier we increment PC, to change where PC starts after the branch we need to get the address of the branch instruction itself
                }
                printf("> BNE Complete\n");
                break;
            }

            case 0x0:{//BEQ
                if(CPU.regs[rs1] == CPU.regs[rs2]){
                    //branch
                    CPU.PC = (CPU.PC - 4) + offset; //since earlier we increment PC, to change where PC starts after the branch we need to get the address of the branch instruction itself
                }
                printf("> BEQ Complete\n");
                break;
            }

            default:
                printf("[Err]: B-type operation not implemented yet\n");
                break;
            }

            break;
        }

        //CSR operations as per ZICSR Extension
        case 0x73:{

            uint16_t CSRInstruction = (instruction & ((uint16_t)(0b111) << 12)) >> 12;
            printf("> CSR type instruction found");

            switch (CSRInstruction){
                case 0x01:{
                    //CSRRW - writes old value of CSR into Rd, and value from rs1 to the CSR.
                    //if rd is 0 though, rs1 is written into CSR but nothing is written into Rd
                    uint16_t rd = (uint16_t)((instruction & ((uint16_t)0b11111 << 7)) >> 7);
                    uint16_t rs1 = (uint16_t)((instruction & ((uint16_t)0b11111 << 15)) >> 15);
                    uint16_t csr = (uint16_t)((instruction & ((uint16_t)0b111111111111 << 20)) >> 20);
                    
                    if(rd != 0){
                        //as long as rd is not 0, then put CSR value into Rd
                        CPU.regs[rd] = load_csr(&CPU, csr);
                        printf("> Old CSR: %d value read and stored into reg: %d\n", csr, rd);
                    }

                    //then put value from rs1 into csr
                    store_csr(&CPU, csr, CPU.regs[rs1]);
                    printf("> New CSR value stored from reg: %d into CSR: %d\n", rs1, csr);
                    printf("> CSRRW instruction completed\n");
                    break;
                }

                case 0x05:{
                    //CSRRWI
                    uint16_t rd = (uint16_t)((instruction & ((uint16_t)0b11111 << 7)) >> 7);
                    uint16_t imm = (uint16_t)((instruction & ((uint16_t)0b11111 << 15)) >> 15);
                    uint16_t csr = (uint16_t)((instruction & ((uint16_t)0b111111111111 << 20)) >> 20);
                    
                    if(rd != 0){
                        //as long as rd is not 0, then put CSR value into Rd
                        CPU.regs[rd] = load_csr(&CPU, csr);
                        printf("> Old CSR: %d value read and stored into reg: %d\n", csr, rd);
                    }

                    store_csr(&CPU, csr, imm);
                    printf("> New CSR value: %d into CSR: %d\n", imm, csr);
                    printf("> CSRRWI instruction completed\n");
                    break;
                }

                case 0x02:{
                    //CSRRS: writes old CSR value into rd, and then rs1 is treated as a bit mask that sets the bits in the CSR (if that bit is writable - SIE taken care by the helper functions)
                    uint16_t rd = (uint16_t)((instruction & ((uint16_t)0b11111 << 7)) >> 7);
                    uint16_t rs1 = (uint16_t)((instruction & ((uint16_t)0b11111 << 15)) >> 15);
                    uint16_t csr = (uint16_t)((instruction & ((uint16_t)0b111111111111 << 20)) >> 20);
                    
                    uint64_t oldcsrvalue = load_csr(&CPU, csr);

                    if(rd != 0){//ensures the 0 register that ISA declares to be always just 0 stays that way
                        CPU.regs[rd] = oldcsrvalue;
                    }

                    if(rs1 != 0){//ISA specifies that if rs1, then no write happens
                        uint64_t newcsrvalue = oldcsrvalue | CPU.regs[rs1];
                        store_csr(&CPU, csr, newcsrvalue);
                    }

                    printf("> New CSR value SET based on reg: %d at CSR: %d\n", rs1, csr);
                    printf("> CSRRS instruction completed\n");
                    break;
                }

                case 0x06:{//CSRRSI
                    uint16_t rd = (uint16_t)((instruction & ((uint16_t)0b11111 << 7)) >> 7);
                    uint16_t imm = (uint16_t)((instruction & ((uint16_t)0b11111 << 15)) >> 15);
                    uint16_t csr = (uint16_t)((instruction & ((uint16_t)0b111111111111 << 20)) >> 20);
                    
                    uint64_t oldcsrvalue = load_csr(&CPU, csr);
                    if(rd != 0){//ensures the 0 register that ISA declares to be always just 0 stays that way
                        CPU.regs[rd] = oldcsrvalue;
                    }

                    if(imm != 0){//ISA specifies that if rs1, then no write happens
                        uint64_t newcsrvalue = oldcsrvalue | imm;
                        store_csr(&CPU, csr, newcsrvalue);
                    }

                    printf("> New CSR value SET based on value: %d at CSR: %d\n", imm, csr);
                    printf("> CSRRSI instruction completed\n");
                    break;
                }

                case 0x03:{
                    //CSRRC: writes old CSR value into rd and then rs1 bit mask clears the csr value bits if it can be set
                    uint16_t rd = (uint16_t)((instruction & ((uint16_t)0b11111 << 7)) >> 7);
                    uint16_t rs1 = (uint16_t)((instruction & ((uint16_t)0b11111 << 15)) >> 15);
                    uint16_t csr = (uint16_t)((instruction & ((uint16_t)0b111111111111 << 20)) >> 20);
                    
                    uint64_t oldcsrvalue = load_csr(&CPU, csr);
                    if(rd != 0){//ensures the 0 register that ISA declares to be always just 0 stays that way
                        CPU.regs[rd] = oldcsrvalue;
                    }

                    if(rs1 != 0){//ISA specifies that if rs1, then no write happens
                        uint64_t newcsrvalue = oldcsrvalue & ~(CPU.regs[rs1]);
                        store_csr(&CPU, csr, newcsrvalue);
                    }
                    
                    printf("> New CSR value CLEARED based on bitmask at reg: %d at CSR: %d\n", rs1, csr);
                    printf("> CSRRC instruction completed\n");
                    break;
                }
                
                case 0x07:{//CSRRCI
                    uint16_t rd = (uint16_t)((instruction & ((uint16_t)0b11111 << 7)) >> 7);
                    uint16_t imm = (uint16_t)((instruction & ((uint16_t)0b11111 << 15)) >> 15);
                    uint16_t csr = (uint16_t)((instruction & ((uint16_t)0b111111111111 << 20)) >> 20);
                    
                    uint64_t oldcsrvalue = load_csr(&CPU, csr);
                    if(rd != 0){//ensures the 0 register that ISA declares to be always just 0 stays that way
                        CPU.regs[rd] = oldcsrvalue;
                    }

                    if(imm != 0){
                        uint64_t newcsrvalue = oldcsrvalue & ~imm;
                        store_csr(&CPU, csr, newcsrvalue);
                    }

                    printf("> New CSR value CLEARED based on value: %d at CSR: %d\n", imm, csr);
                    printf("> CSRRCI instruction completed\n");
                    break;
                }

                default:{
                    printf("[Err]: CSR modificaiton instruction found, but actual funciton (CSRRW, CSRRI etc) not recognized\n");
                    break;
                }
            }

            break;
        }
        

        default:{
            printf("[Err]: I-type operation NOT IMPLEMENTED YET\n");
            break;
        }
        }


        //EXECUTE (+ EVAL ADRESSES)
       
        //----TESTING-----// //REMOVE AFTER IMPLEMENTING ECALL
        if(CPU.PC == prevPC){
            //this means the program has entered a infinite loop and so we end the process and break out of the processor loop
            printf("[WARNING]: INFINITE LOOP\n");
            break;
        }

        prevPC = CPU.PC;

    }

    DeInitBUS(CPU.bus);
    printf("***Final State of Registers:***\n");
    dump_regs(&CPU);

    return 0;

}
