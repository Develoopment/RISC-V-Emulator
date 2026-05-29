#a makefile is a way to streamline building and cleaning my project without having to say "gcc ..."
#it also checks for changed files and compiles only the changed parts which allows for faster compile times every time I build my project

CC = gcc #compiler call
CFLAGS = -Wall -Wextra -std=c11 # flags to enable warnings and use C11 conventions

#this is the linking step
emulator: main.o dram.o bus.o
	$(CC) $(CFLAGS) main.o dram.o bus.o -o emulator 

#the -c means compile only, don't link
main.o: main.c dram.h bus.h
	$(CC) $(CFLAGS) -c main.c 

dram.o: dram.c dram.h 
	$(CC) $(CFLAGS) -c dram.c

bus.o: bus.c bus.h
	$(CC) $(CFLAGS) -c bus.c

clean:
	rm -f *.o emulator *.elf *.bin

prepare_program: test.s
	riscv64-unknown-elf-as -march=rv64i -o test.o test.s
	riscv64-unknown-elf-ld -Ttext=0x0 -o test.elf test.o