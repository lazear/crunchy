#crunchy makefile
#2007-2016, Michael Lazear

FINAL	= new
OBJS	= *.o				# Elf object files
CC	    = /home/lazear/opt/cross/bin/i686-elf-gcc
LD		= /home/lazear/opt/cross/bin/i686-elf-ld
NASM		= nasm
AS 		= ~/opt/cross/bin/i686-elf-as
AR		= /home/lazear/opt/cross/bin/i686-elf-ar
CP		= cp
CCFLAGS	= -w -fno-builtin -nostdlib -ffreestanding -std=gnu99 -m32 --sysroot=. -c 
ASFLAGS = -f elf



compile:
	#Compile C source
	$(CC) $(CCFLAGS) usr/string/*.c	
	$(CC) $(CCFLAGS) usr/stdlib/*.c
	$(CC) $(CCFLAGS) usr/stdio/*.c
	$(CC) $(CCFLAGS) usr/*.c

	#Generate archive
	$(AR) -cvru libx.a *.o

	$(NASM) $(ASFLAGS) crt0.s
	$(AS) crti.s -o crti.o
	$(AS) crtn.s -o crtn.s

	cp libx.a usr/lib
	cp crt*.o usr/lib
	cp usr/lib/libx.a usr/lib/libc.a

	
clean:
	rm *.o				# Delete all of the object files
	rm *.a
	
	

	