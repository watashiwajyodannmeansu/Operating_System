.PHONY: all img clean

all: clean img
	echo ""
img: os.img
	echo ""
bootloader.o: boot/bootloader.asm
	as --32 boot/bootloader.asm -o bootloader.o

bootloader.elf: bootloader.o boot/bootloader.lds
	ld -m elf_i386 -T boot/bootloader.lds -o bootloader.elf bootloader.o

bootloader: bootloader.elf
	objcopy -O binary -j .boot bootloader.elf bootloader

kernel.o: boot/kernel.c
	gcc -m64 -ffreestanding -fno-pie -fno-pic -fno-stack-protector -mno-sse -mno-mmx -fno-asynchronous-unwind-tables -fno-unwind-tables -c boot/kernel.c -o kernel.o

kernel_entry.o: boot/kernel_entry.asm
	as --64 boot/kernel_entry.asm -o kernel_entry.o

kernel.elf: kernel_entry.o kernel.o boot/kernel.lds
	ld -T boot/kernel.lds -o kernel.elf kernel_entry.o kernel.o

kernel: kernel.elf
	objcopy -O binary kernel.elf kernel

bin:


os.img: bootloader kernel bin
	dd if=/dev/zero of=os.img bs=1M count=10

	dd if=bootloader of=os.img bs=512 count=1 conv=notrunc
	dd if=boot/bootloader.header of=os.img oseek=37 bs=512
	dd if=boot/bootloader.name of=os.img oseek=38 bs=512
	dd if=boot/bootloader.sectorfile of=os.img oseek=39 bs=512

	dd if=kernel of=os.img oseek=2 bs=512 count=32
	dd if=boot/kernel.header of=os.img oseek=1 bs=512
	dd if=boot/kernel.name of=os.img oseek=35 bs=512
	dd if=boot/kernel.sectorfile of=os.img oseek=36 bs=512

	dd if=boot/bootdir.header of=os.img oseek=40 bs=512
	dd if=boot/bootdir.name of=os.img oseek=41 bs=512
	dd if=boot/bootdir.sectorfile of=os.img oseek=42 bs=512

	dd if=bin/bindir.header of=os.img oseek=43 bs=512
	dd if=bin/bindir.name of=os.img oseek=44 bs=512
	dd if=bin/bindir.sectorfile of=os.img oseek=45 bs=512

	dd if=root/rootdir.header of=os.img oseek=46 bs=512
	dd if=root/rootdir.name of=os.img oseek=47 bs=512
	dd if=root/rootdir.sectorfile of=os.img oseek=48 bs=512

clean:
	rm -f bootloader.o bootloader.elf bootloader kernel.o kernel_entry.o kernel.elf kernel os.img bootloader.ld