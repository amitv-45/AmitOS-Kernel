# Variables
CC = gcc
AS = as
LNK = gcc
CFLAGS = -m32 -c -ffreestanding -O2 -Wall -Wextra -fno-stack-protector
LDFLAGS = -m32 -T linker.ld -ffreestanding -O2 -nostdlib -lgcc

# Targets
all: myos.bin

boot.o: boot.s
	$(AS) --32 boot.s -o boot.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

myos.bin: boot.o kernel.o
	$(LNK) $(LDFLAGS) -o myos.bin boot.o kernel.o

clean:
	rm -f *.o myos.bin