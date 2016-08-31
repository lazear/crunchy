/*
kernel.c
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

#include <kernel.h>
#include <vga.h>
#include <x86.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include <mutex.h>
#include <proc.h>
#include <assert.h>
#include <ide.h>
#include <ext2.h>
#include <elf.h>

char logo[] = {0x5f,0x5f,0x5f,0x2e,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5f,0x5f,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2e,0x5f,0x5f,0x20,0x20,0x20,0x0a,0x5c,0x5f,0x20,0x7c,0x5f,0x5f,0x20,0x5f,0x5f,0x5f,0x5f,0x5f,0x20,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x20,0x20,0x20,0x5f,0x5f,0x5f,0x5f,0x20,0x20,0x20,0x5f,0x5f,0x5f,0x5f,0x5f,0x20,0x20,0x20,0x5f,0x5f,0x5f,0x5f,0x5f,0x2f,0x20,0x20,0x7c,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x20,0x20,0x7c,0x20,0x20,0x7c,0x20,0x20,0x0a,0x20,0x7c,0x20,0x5f,0x5f,0x20,0x5c,0x5c,0x5f,0x5f,0x20,0x20,0x5c,0x5c,0x5f,0x20,0x20,0x5f,0x5f,0x20,0x5c,0x5f,0x2f,0x20,0x5f,0x5f,0x20,0x5c,0x20,0x2f,0x20,0x20,0x20,0x20,0x20,0x5c,0x5f,0x2f,0x20,0x5f,0x5f,0x20,0x5c,0x20,0x20,0x20,0x5f,0x5f,0x5c,0x5f,0x5f,0x20,0x20,0x5c,0x20,0x7c,0x20,0x20,0x7c,0x20,0x20,0x0a,0x20,0x7c,0x20,0x5c,0x5f,0x5c,0x20,0x5c,0x2f,0x20,0x5f,0x5f,0x20,0x5c,0x7c,0x20,0x20,0x7c,0x20,0x5c,0x2f,0x5c,0x20,0x20,0x5f,0x5f,0x5f,0x2f,0x7c,0x20,0x20,0x59,0x20,0x59,0x20,0x20,0x5c,0x20,0x20,0x5f,0x5f,0x5f,0x2f,0x7c,0x20,0x20,0x7c,0x20,0x20,0x2f,0x20,0x5f,0x5f,0x20,0x5c,0x7c,0x20,0x20,0x7c,0x5f,0x5f,0x0a,0x20,0x7c,0x5f,0x5f,0x5f,0x20,0x20,0x28,0x5f,0x5f,0x5f,0x5f,0x20,0x20,0x2f,0x5f,0x5f,0x7c,0x20,0x20,0x20,0x20,0x5c,0x5f,0x5f,0x5f,0x20,0x20,0x3e,0x5f,0x5f,0x7c,0x5f,0x7c,0x20,0x20,0x2f,0x5c,0x5f,0x5f,0x5f,0x20,0x20,0x3e,0x5f,0x5f,0x7c,0x20,0x28,0x5f,0x5f,0x5f,0x5f,0x20,0x20,0x2f,0x5f,0x5f,0x5f,0x5f,0x2f,0x0a,0x20,0x20,0x20,0x20,0x20,0x5c,0x2f,0x20,0x20,0x20,0x20,0x20,0x5c,0x2f,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5c,0x2f,0x20,0x20,0x20,0x20,0x20,0x20,0x5c,0x2f,0x20,0x20,0x20,0x20,0x20,0x5c,0x2f,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x5c,0x2f,0x20,0x20,0x20,0x20,0x20, '\n'};

static mutex key_mutex = { .lock = 0 };

extern STREAM* kb;

extern void scheduler();
void scheduler() {}
extern void switch_to_user(void* (fn)(), uint32_t esp);
//We enter into kernel initialize with the GDT and IDT already loaded, and interrupts disabled

extern uint32_t* KERNEL_PAGE_DIRECTORY;
uint32_t KERNEL_END = 0;
extern uint32_t _init_pt[];
extern uint32_t _init_pd[];
extern uint32_t stack_top;
extern uint32_t stack_bottom;




void kernel_initialize(uint32_t kernel_end) {

	/*
	1.	Initialize physical memory manager, with private heap @ kernel_end to 2MB
			- Bitmap has the first 2MB (512 bits) marked as used, will not be allocated
			- Allows k_page_alloc()
	2.	Initialize paging, passing the reserved first page directory address
	3.	Initialize heap management with malloc, free, and sbrk.
			- Utilizes both k_page_alloc() and k_paging_map() to generate a continous
				virtual address space.
			- Public heap starts at 3GB. This should be changed when higher half is implemented
	4.	Todo - initialize multithreading.
	*/

	KERNEL_END = kernel_end;
	k_mm_init(kernel_end);
	k_heap_init();
	k_paging_init(_init_pd);

	keyboard_install();
	timer_init();
	syscall_init();

	sti();

	vga_setcolor(VGA_COLOR(VGA_WHITE, VGA_BLACK));
	vga_clear();
	vga_pretty(logo, VGA_LIGHTGREEN);

	ide_init();
	buffer_init();

//	lsroot();
	//sb_dump(ext2_superblock(1));
	//inode_dump(ext2_inode(1,11));

	
	uint32_t* data = ext2_open(ext2_inode(1,12));
/*		for (int i =0; i < 30; i++) {
		printf("%x ", byte_order(data[i]));
	}*/
	//char* data = & _binary_app_sso_start;
	elf32_ehdr * ehdr = (elf32_ehdr*) data; 
	assert(ehdr->e_ident[0] == ELF_MAGIC);
	printf("r->e_ident %x\t",ehdr->e_ident[0]);     
	printf("dr->e_type %x\t", ehdr->e_type);                
	printf(">e_machine %x\n", ehdr->e_machine);              
	printf(">e_version %x\t",ehdr->e_version);              
	printf("r->e_entry %x\t",ehdr->e_entry);                
	printf("r->e_phoff %x\n",ehdr->e_phoff);                
	printf("r->e_shoff %x\t",ehdr->e_shoff);                
	printf("r->e_flags %x\t",ehdr->e_flags);           
	printf("->e_ehsize %x\n", ehdr->e_ehsize);               
	printf("_phentsize %x\t", ehdr->e_phentsize);            
	printf("r->e_phnum %x\t", ehdr->e_phnum);                
	printf("_shentsize %x\n", ehdr->e_shentsize);            
	printf("r->e_shnum %x\t", ehdr->e_shnum);               
	printf("e_shstrndx %x\n", ehdr->e_shstrndx);    


	elf32_phdr* phdr = (elf32_phdr*) ((uint32_t) data + ehdr->e_phoff);
	printf("LOAD: off 0x%x vaddr 0x%x paddr 0x%x filesz 0x%x memsz 0x%x\n",
		 				phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz);
	k_paging_map(k_page_alloc(), phdr->p_vaddr, 0x7);
	memcpy(phdr->p_vaddr, (uint32_t)data + phdr->p_offset, phdr->p_memsz);
	phdr++;
	printf("LOAD: off 0x%x vaddr 0x%x paddr 0x%x filesz 0x%x memsz 0x%x\n",
		 				phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz);
	k_paging_map( k_page_alloc(), phdr->p_vaddr, 0x7);
	memcpy(phdr->p_vaddr, (uint32_t)data + phdr->p_offset, phdr->p_memsz);
	



	
	void (*entry)(void);
	entry = (void(*)(void))(ehdr->e_entry);

	printf("entry: %x\n", entry);

	entry();


	//int i = ext2_touch("data", 'Hello, World!', 13);

	lsroot();

//	asm volatile ("int $0x80");
	for(;;);
}


