//vga.c

#include <vga.h>
#include <types.h>

char* VGA_MEMORY = 0x000B8000;


int CURRENT_X = 0;
int CURRENT_Y = 0;
int CURRENT_ATTRIB = 0;

int vga_current_x() { return CURRENT_X; }
int vga_current_y() { return CURRENT_Y; }

/* Move the screen's cursor to the specified pos and line 
 * x is the char position, y is the line */ 
void vga_move_cursor( uint16_t x, uint16_t y ) {
	unsigned temp;
	temp = (y * 80) + x;

	outportb(0x3D4, 14);
	outportb(0x3D5, temp >> 8);
	outportb(0x3D4, 15);
	outportb(0x3D5, temp);
}

void vga_update_cursor() {
	vga_move_cursor(CURRENT_X/2, CURRENT_Y);
}

void vga_setcolor(int attrib) {
	CURRENT_ATTRIB = attrib;
}

void vga_clear() {
	char* vga_address = VGA_MEMORY;

	const long size = 80 * 25;
	for (long i = 0; i < size; i++ ) { 
		*vga_address++ = 0; // character value
		*vga_address++ = CURRENT_ATTRIB; // color value
	}
}

void vga_overwrite_color(int color, int start_x, int start_y, int end_x, int end_y) {
	char *vga_address = VGA_MEMORY;
	int sizeend = 2*end_x + (160* end_y);
	//const uint32_t size = 80*25;

	for (int i = (start_x+1 + (160*start_y)); i < sizeend; i += 2) {
		
		vga_address[i] = color;
	}
}

// kernel level putc, designate x and y position
void vga_kputc(char c, int x, int y) {
	char *vga_address = VGA_MEMORY + (x + y * 160);
	*vga_address = c | (CURRENT_ATTRIB << 8);
}

void vga_puts(char* s) {
	int i = 0;
	while (*s != 0) {
		vga_putc(*s);
		*s++;

	}
}

// Automatically update text position, used in vga_puts
void vga_putc(char c) {
	if (c == '\n') {
		CURRENT_Y += 1;
		CURRENT_X = 0;
		return;
	}
	vga_kputc(c, CURRENT_X, CURRENT_Y);
	CURRENT_X += 2;
	if (CURRENT_X >= 160) {
		CURRENT_X = 0;
		CURRENT_Y += 1;
	}
	vga_update_cursor();

}

void vga_pretty(char* s, int color) {
	int start_x = CURRENT_X;
	int start_y = CURRENT_Y;

	vga_puts(s);

	vga_overwrite_color(color, start_x, start_y, CURRENT_X, CURRENT_Y);

}