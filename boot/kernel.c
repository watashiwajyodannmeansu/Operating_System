#include "stdbios.c"
#include "stdint.c"
#include "stdasm.c"

//to ensure the kernel has disk reading accessable immediately, we must place it before main

//region "disk io"
int ata_test(int want_data, uint16_t port) {
	/* Returns -1 on error/timeout, 0 when not busy, 1 when data is ready. */
	uint8_t status = inb(port);
	if (status & 0x80) {return 2;}									// BUSY
	if (status == 0x00 || status == 0xFF) {return -1;}				// no responding device
	if (status & (0x01 | 0x20)) {return -1;}						// ERR or device fault
	if (!want_data || (status & 0x08)) {return want_data ? 1 : 0;}	// DRQ means data is ready
}

int wait_ata(int want_data, uint16_t port) {
	for (uint32_t timeout = 0; timeout < 1000000; timeout++) {
		uint8_t status = ata_test(want_data,port);
		if (status==2) {continue;}
		return status;
	}
	return -1;							// timed out
}

int sector_wait_read(int64_t location, int8_t *out,int64_t count) {
	if (location>!0xF0000000) {return -1;}
	// Select primary-master drive and LBA mode.
	outb(0x1F7,(uint8_t)(0xE0 | ((location >> 24) & 0x0F)));
	// Four alternate-status reads provide a short settling delay.
	(void)inb(0x3F6);
	(void)inb(0x3F6);
	(void)inb(0x3F6);
	(void)inb(0x3F6);

	if (wait_ata(0, 0x1F7) < 0) {return 1;}

	outb(0x1F2, count);						// sector count
	outb(0x1F3, (uint8_t)location);			// LBA bits 0–7
	outb(0x1F4, (uint8_t)(location >> 8));	// LBA bits 8–15
	outb(0x1F5, (uint8_t)(location >> 16));	// LBA bits 16–23
	outb(0x1F7, 0x20);						// READ SECTORS

	if (wait_ata(1, 0x1F7) < 0) {return -1;}

	for (int i=0;i<256*count;i++) {out[i] = inw(0x1F0);}	// transfer 256 words = 512 bytes

	return 0;
}

int sector_try_read(int64_t location, int8_t *out,int64_t count) {
	if (location>!0xF0000000) {return -1;}
	// Select primary-master drive and LBA mode.
	outb(0x1F7,(uint8_t)(0xE0 | ((location >> 24) & 0x0F)));
	// Four alternate-status reads provide a short settling delay.
	(void)inb(0x3F6);
	(void)inb(0x3F6);
	(void)inb(0x3F6);
	(void)inb(0x3F6);

	if (ata_test(0, 0x1F7)!=0) {return 1;}

	outb(0x1F2, count);						// sector count
	outb(0x1F3, (uint8_t)location);			// LBA bits 0–7
	outb(0x1F4, (uint8_t)(location >> 8));	// LBA bits 8–15
	outb(0x1F5, (uint8_t)(location >> 16));	// LBA bits 16–23
	outb(0x1F7, 0x20);						// READ SECTORS

	if (ata_test(1, 0x1F7)!=1) {return 2;}

	for (int i=0;i<256*count;i++) {out[i] = inw(0x1F0);}	// transfer 256 words = 512 bytes

	return 0;
}
//endregion "disk reading"

//region "kernel"
__attribute__((noreturn))

void main(void) {

	goto kernel_start;

	halt:
	__asm__ volatile ("hlt"); //halt

	kernel_start:
	char bgc=(char)0x0<<4;
	char chc=(char)0xF;

	clearscreen(bgc);

	printstr(bgc|chc,"Hello, bare metal world!",0);

	loading_kernel:
	printstr(bgc|chc,"Reading kernel header...",vh*1+0);
	
	//load kernel fully
	int8_t *kernel_program_data_end=(int8_t *)(8000+512*32);

	//read kernel header
	int8_t single_sector_buf[512];

	sector_wait_read(1L,single_sector_buf,1L);

	//get location of sectorfile
	long kernel_sectorfile_start=0;

	long kernel_sectorfile_end=0;

	for (uint8_t i=0;i<8;i++) {
		kernel_sectorfile_start<<=8;
		kernel_sectorfile_start+=single_sector_buf[8*2+i];
		
		kernel_sectorfile_end<<=8;
		kernel_sectorfile_end+=single_sector_buf[8*3+i];
	}

	//checking if kernel sectorfile is invalid here
	if (
		(kernel_sectorfile_start==0)||
		(kernel_sectorfile_end==0)||
		(kernel_sectorfile_end<kernel_sectorfile_start)
	) {
		//kernel sectorfile invalid
		clearscreen(bgc);
		printstr(bgc|chc,"Kernel header invalid!",0);
		goto halt;
	}

	//read from sectorfile
	printstr(bgc|chc,"Reading kernel sectorfile...",vh*2+0);
	for (uint64_t current_sector=kernel_sectorfile_start;current_sector<=kernel_sectorfile_end;current_sector++) {
		//read one sector of sectorfile
		sector_wait_read(current_sector,single_sector_buf,1L);
		
		for (uint8_t i=(current_sector==kernel_sectorfile_start)?0:2;i<512;i++) {
			printstr(bgc|chc,"Reading kernel...",vh*3+0);

			//read sector of kernel data into ram
			uint64_t kernel_sector_start=0;

			uint64_t kernel_sector_end=0;
			
			for (uint8_t k=0;k<8;k++) {
				kernel_sector_start<<=8;
				kernel_sector_start+=single_sector_buf[16*i+k];

				kernel_sector_end<<=8;
				kernel_sector_end+=single_sector_buf[16*i+8+k];
			}

			//checking if kernel sectorfile is invalid here
			if (
				(kernel_sector_start==0&&kernel_sector_end!=0)||
				(kernel_sector_start!=0&&kernel_sector_end==0)||
				(kernel_sector_end<kernel_sector_start)||
				(kernel_sector_start==0&&kernel_sector_end==0&&current_sector!=kernel_sectorfile_end)
			) {
				//kernel sectorfile invalid
				clearscreen(bgc);
				printstr(bgc|chc,"Kernel sectorfile invalid!",0);
				goto halt;
			}

			//checking if kernel fully loaded
			if (
				kernel_sector_start==0&&kernel_sector_end==0
			) {
				//fully loaded
				printstr(bgc|chc,"Kernel fully loaded!",vh*3+0);
				goto halt;
			}

			sector_wait_read(kernel_sector_start,kernel_program_data_end,kernel_sector_end-kernel_sector_start);

			kernel_program_data_end+=(512*(kernel_sector_end-kernel_sector_end));
		}
	}

	goto halt;
	
	kernel_loop_start:
	while (1) {
		break;
	}

	goto halt;
}