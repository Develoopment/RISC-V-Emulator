//this is going to be the actual CPU
#include <stdio.h>
#include <stdint.h>

//this struct holds the STATE of the CPU
//1. State declaration
typedef struct{
    uint64_t regs[32];  //initiating all to 0 for testing purposes
    uint64_t PC; //starting Program Counter at 0x0000
    uint8_t mem[256]; //uint8_t becuase memory is byte addressable.
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

    cpu CPU = {0};

    //-------TESTING---------//
    CPU.regs[6] = 2;
    CPU.regs[7] = 4;

    //REPS ADD R10, R6, R7
    CPU.mem[0] = 0x33;
    CPU.mem[1] = 0x05;
    CPU.mem[2] = 0x73;
    CPU.mem[3] = 0x00;

    //2. Populating special registers
    //according to the RISC V ISA reg 2 is the SP and reg 0 is hardwired to be all 0s
    CPU.regs[0] = 0;
    CPU.regs[2] = 256; //letting it point to the last memory cell, since stack grows downward, any nonzero value here would work

    //3. Instruction Cycle (managed by the FSM in the datapath)
    //the instruction cycle is composed of: fetch instruction, decode instruction, eval address, fetch operands, execute, store value
    while(CPU.PC < 10){

        //FETCH INSTRUCTION
        uint32_t instruction = (uint32_t)CPU.mem[CPU.PC] | (uint32_t)CPU.mem[CPU.PC+1] << 8 | (uint32_t)CPU.mem[CPU.PC+2] << 16 | (uint32_t)CPU.mem[CPU.PC+3] << 24; //byte addressable where values are stored LSB at lowest address (lilttle endian)
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
                    printf("ADD executed\n");
                    break;
                }

                default:{
                    printf("R-type operation NOT IMPLEMENTED YET\n");
                    break;
                }
            }
            
            break;
        }

        case 0x13:{ //I type
            
            break; 
        }
        
        default:{
            printf("I-type operation NOT IMPLEMENTED YET\n");
            break;
        }
        }


        //EXECUTE (+ EVAL ADRESSES)
       
    


    }

    dump_regs(&CPU);

    return 0;

}
