/*
===============================================================================
MIT License
Copyright (c) 2007-2016 Michael Lazear

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
===============================================================================
*/

/*
http://www.intel.com/design/pentium/datashts/24201606.pdf

BSP: Bootstrap processor - the core we are currently running on
AP: Application processor(s). Currently halted.
*/
#include <types.h>
#include <acpi.h>
#include <lapic.h>
#include <traps.h>
#include <x86.h>
#include <mutex.h>

#define INIT       0x00000500   // INIT/RESET
#define STARTUP    0x00000600   // Startup IPI
#define DELIVS     0x00001000   // Delivery status
#define ASSERT     0x00004000   // Assert interrupt (vs deassert)
#define DEASSERT   0x00000000
#define LEVEL      0x00008000   // Level triggered
#define BCAST      0x00080000   // Send to all APICs, including self.
#define BUSY       0x00001000
#define FIXED      0x00000000

volatile uint32_t* LAPIC = NULL;




static void lapic_write(int index, int value) {
	LAPIC[index/4] = value;		// div by 4 for sizeof(int)
	LAPIC[0x8];					// Wait by reading;
}

static uint32_t lapic_read(int index) {
	return LAPIC[index/4];
}

void lapic_eoi() {
	if (!LAPIC)
		return;
	lapic_write(LAPIC_EOI, 0);
}

/* Initilize the local advanced programmable interrupt chip */
void lapic_init() {

	/* Use default Local APIC location */
	LAPIC = LAPIC_BASE;

	/* Paging is enabled, so we need to map in the physical mem */
	if (k_phys_to_virt(LAPIC_BASE))
		vga_pretty("LAPIC_BASE is already mapped\n", 4);
	k_paging_map(LAPIC_BASE, LAPIC_BASE, 0x3);

	printf("LAPIC id: %d\n", lapic_read(LAPIC_ID) >> 24);

	return;
	//pic_disable();
	/* Enable local APIC and set the spurious interrupt vector */
	lapic_write(LAPIC_SIV, 0x100 | (IRQ0 + IRQ_SPURIOUS));
	// Calibrate timer
	lapic_write(LAPIC_TDCR, 0x00020000 | 0x20);	// Irq0 + IRQ_timer | Periodic
	lapic_write(LAPIC_TICR, 10000000);

	/* Mask interrupts */
	lapic_write(LAPIC_LINT0, 0x10000);
	lapic_write(LAPIC_LINT1, 0x10000);

	if ((lapic_read(LAPIC_VER) >> 16) & 0xFF >= 4)
		lapic_write(0x0340, 0x10000);

	lapic_write(LAPIC_ERR, 0);
	lapic_write(LAPIC_ERR, 0);
	lapic_write(LAPIC_EOI, 0);	// Clear any existing interrupts

	lapic_write(LAPIC_ICRHI, 0);
	lapic_write(LAPIC_ICRLO, INIT | LEVEL | BCAST);
	while(LAPIC[LAPIC_ICRLO] & DELIVS)
		;

	lapic_write(LAPIC_TPR, 0);
}


mutex proc_m = {.lock = 0};

int udelay(int i) {
	
}

/* Sends a start up IPI to the AP processor designated <apic_id>,
telling it to start executing code at <address> 
Address must be page-aligned, and in the first megabyte of system mem*/
void lapic_start_AP(int apic_id) {

	/* Following Intel SMP startup procedure:
	Write 0Ah to CMOS RAM location 0Fh (shutdown code). 
	This initializes a warm reset
	*/
	outb(0x70, 0xF);
	outb(0x71, 0xA);

	/*
	The STARTUP IPI causes the target processor to start executing in Real Mode from address
	000VV000h, where VV is an 8-bit vector that is part of the IPI message. Startup vectors are
	limited to a 4-kilobyte page boundary in the first megabyte of the address space. Vectors A0-BF
	are reserved; do not use vectors in this range. 
	*/
	uint16_t* warm_reset_vector = (uint16_t) P2V(0x40 << 4 | 0x67);
	warm_reset_vector[0] = 0;
	warm_reset_vector[1] = 0x8000 >> 4;
	//*warm_reset_vector = 0x8000;

	lapic_write(LAPIC_ICRHI, apic_id << 24);
	lapic_write(LAPIC_ICRLO, INIT | LEVEL | ASSERT);
	//udelay(2);
	lapic_write(LAPIC_ICRLO, INIT | LEVEL);
	udelay(1);
	printf("Attemping LAPIC startup: %d: %x\n", apic_id, 0x8000);
	for (int i = 0; i < 2; i++) {
		lapic_write(LAPIC_ICRHI, apic_id << 24);
		lapic_write(LAPIC_ICRLO, STARTUP |  0x8000 >> 12);
		udelay(1);
	}

}


extern uint32_t _binary_ap_entry_start[];
extern uint32_t _binary_ap_entry_end[];
extern uint32_t KERNEL_PAGE_DIRECTORY;

struct cpu {
	uint8_t id;
	uint32_t stack;
	uint32_t blah;
	struct segdesc gdt[8];
} cpus[8];

static int ncpu = 0;
extern struct cpu *cpu asm("%gs:0");


void mp_enter() {
//	asm volatile("cli");
//	lapic_init();
	acquire(&proc_m);
	ncpu++;
	printf("cpu %d initializing\n", ncpu);
	printf("eflags: %x\n", get_eflags());
	printf("%x\n", mp_processor_id());

	printf("eflags: %x\n", get_eflags());
	release(&proc_m);
	for(;;);
}

int mp_processor_id() {
	//if interrupts enabled, panic
	if (get_eflags() & 0x200) {
		panic("INTERRUPTS ENABLED");
		return -1;
	}
	return lapic_read(LAPIC_ID) >> 24;
}


int mp_number_of_processors() {
	return ncpu;	
}

void ap_entry_init() {

	int ap_entry_size = (uint32_t) _binary_ap_entry_end - (uint32_t) _binary_ap_entry_start;
	
	uint8_t* code = 0x8000;
	uint8_t* stack = mm_alloc(0x10000);
	// Pass several arguments to the ap-start up code
	*(void**)(code - 4) = stack + 0x10000;
	*(void**)(code - 8) = mp_enter;
	*(int**)(code - 12) = V2P(KERNEL_PAGE_DIRECTORY);

	memmove(code, _binary_ap_entry_start, ap_entry_size);


}