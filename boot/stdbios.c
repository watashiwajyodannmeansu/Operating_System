//define the pointer to the vga
volatile unsigned short *vga = (volatile unsigned short *)0xB8000;

int printstr(char col, char *str, unsigned int offset) {
	for (unsigned short i=0;str[i]!='\0';i++) {
		vga[offset+i]=(((unsigned short)col)<<8)|str[i];
	}
	return 0;
}

unsigned short int vh=24;
unsigned short int vw=80;

int clearscreen(char col) {
	for (unsigned short int i=0;i<vh*vw;i++) {
		printstr(col," ",i);
	}
}