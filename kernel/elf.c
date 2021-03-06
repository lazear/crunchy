/*
elf.c
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

#include <assert.h>
#include <types.h>
#include <ext2.h>
#include <ide.h>
#include <elf.h>

static struct ksym {
	uint32_t address;
	char name[32];
	int size;
} *kernel_symbols;

static int no_ksym = 0;

void build_ksyms(void) {

	int ki = find_inode_in_dir("kernel.bin", 2);

	char* data = ext2_open(ext2_inode(1, ki));

	elf32_ehdr *ehdr = (elf32_ehdr*) data;

	/* Make sure the file ain't fucked */
	assert(ehdr->e_ident[0] == ELF_MAGIC);
	assert(ehdr->e_machine 	== EM_386);
	assert(ehdr->e_type		== ET_EXEC);

	/* Parse the section headers */
	elf32_shdr* shdr 		= ((uint32_t) data) + ehdr->e_shoff;
	elf32_shdr* sh_str		= (uint32_t) shdr + (ehdr->e_shentsize * ehdr->e_shstrndx);
	elf32_shdr* last_shdr 	= (uint32_t) shdr + (ehdr->e_shentsize * ehdr->e_shnum);

	elf32_shdr* strtab 		= NULL;
	elf32_shdr* symtab		= NULL;

	char* string_table 		= (uint32_t) data + sh_str->sh_offset;

	shdr++;					// Skip null entry

	while (shdr < last_shdr) {	
		if (strcmp(".symtab", string_table + shdr->sh_name) == 0) {
			symtab = shdr;
		}
		if (strcmp(".strtab", string_table + shdr->sh_name) == 0) {
			strtab = shdr;
		}
		shdr++;
	}

	if (!strtab || !symtab) {
		vga_pretty("ERROR: Could not load symbol table", 0x4);
		return;
	}

	elf32_sym* sym 		= ((uint32_t) data) + symtab->sh_offset;
	elf32_sym* last_sym = (uint32_t) sym + symtab->sh_size;
	void* strtab_d 		= ((uint32_t) data) + strtab->sh_offset;

	size_t sym_tab_sz = ((uint32_t) last_sym - (uint32_t) sym)/ sizeof(elf32_sym);
	kernel_symbols = malloc(sizeof(struct ksym) * sym_tab_sz);
	/* Output symbol information*/
	int q = 0;
	while(sym < last_sym) {
		if (sym->st_name && sym->st_value > KERNEL_VIRT) {
			
			if ( q > sym_tab_sz)
				printf("ERROR\n");
			char* tmp = (char*) (sym->st_name + (uint32_t)strtab_d);
			//printf("%s, %d\n", tmp, sym->st_size);
			kernel_symbols[q].address = sym->st_value;
			kernel_symbols[q].size = sym->st_size;
			for (int i = 0; i < 32, i < strlen(tmp); i++)
				kernel_symbols[q].name[i] = tmp[i];
			q++;
		}
		sym++;
	}
	no_ksym  = q;
	free(data);
}

char* ksym_find(uint32_t addr) {

	if (!kernel_symbols)
		return NULL;
	int closest_diff = 0x1000;
	int match = 0;

	for (int i = 0; i < no_ksym; i++) {
		int diff = addr - kernel_symbols[i].address;
		if (diff < closest_diff && diff > 0) {
			match = i;
			closest_diff = diff;
		}
	}
	return kernel_symbols[match].name;
}


void* elf_objdump(void* data) {
	elf32_ehdr *ehdr = (elf32_ehdr*) data;

	/* Make sure the file ain't fucked */
	assert(ehdr->e_ident[0] == ELF_MAGIC);
	assert(ehdr->e_machine 	== EM_386);

	char *types[] = { "NONE", "RELOCATABLE", "EXECUTABLE", "SHARED", "CORE"};

	printf("OBJDUMP\n");
	printf("ELF ident\t%x\t",ehdr->e_ident[0]);     
	printf("Type %s\t", types[ehdr->e_type]);                
	printf("Machine %s\n", "i386");              
	printf("Version \t%x\t",ehdr->e_version);              
	printf("Entry\t%x\t",ehdr->e_entry);                
         
	printf("Flags\t%x\n",ehdr->e_flags);           
	

	/* Parse the program headers */
	elf32_phdr* phdr 		= (uint32_t) data + ehdr->e_phoff;
	elf32_phdr* last_phdr 	= (uint32_t) phdr + (ehdr->e_phentsize * ehdr->e_phnum);
	while(phdr < last_phdr) {
		printf("LOAD:\toff 0x%x\tvaddr\t0x%x\tpaddr\t0x%x\n\t\tfilesz\t%d\tmemsz\t%d\talign\t%d\t\n",
		 	phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz, phdr->p_align);
		phdr++;
	} 

	//uint32_t* buf = ext2_file_seek(ext2_inode(1,14), BLOCK_SIZE, ehdr->e_shoff);

	/* Parse the section headers */
	elf32_shdr* shdr 		= ((uint32_t) data) + ehdr->e_shoff;
	elf32_shdr* sh_str		= (uint32_t) shdr + (ehdr->e_shentsize * ehdr->e_shstrndx);
	elf32_shdr* last_shdr 	= (uint32_t) shdr + (ehdr->e_shentsize * ehdr->e_shnum);

	elf32_shdr* strtab 		= NULL;
	elf32_shdr* symtab		= NULL;
	elf32_shdr* reltab		= NULL;

	char* string_table 		= (uint32_t) data + sh_str->sh_offset;

	shdr++;					// Skip null entry
	int q = 1;

	printf("Idx %19s Size\t Address    Offset    Align Type\n", "Name");
	while (shdr < last_shdr) {	
		printf("%2d:%20s %#8x %#8x %5d %#5x %d\n", 
			q++, (string_table + shdr->sh_name), shdr->sh_size,
			shdr->sh_addr, shdr->sh_offset, shdr->sh_addralign, shdr->sh_type);
		if (strcmp(".symtab", string_table + shdr->sh_name) == 0 && shdr->sh_type == SHT_SYMTAB)
			symtab = shdr;
		if (strcmp(".strtab", string_table + shdr->sh_name) == 0 && shdr->sh_type == SHT_STRTAB)
			strtab = shdr;
		if (strcmp(".rel.text", string_table + shdr->sh_name) == 0 && shdr->sh_type == SHT_REL)
			reltab = shdr;
		shdr++;
	}

	if (!strtab || !symtab) {
		vga_pretty("ERROR: Could not load symbol table", 0x4);
		return;
	}

	elf32_sym* sym 		= ((uint32_t) data) + symtab->sh_offset;
	elf32_sym* last_sym = (uint32_t) sym + symtab->sh_size;
	void* strtab_d 		= ((uint32_t) data) + strtab->sh_offset;

	/* Output symbol information*/
	

/* BEGIN RELOCATION CODE */	
	while(sym < last_sym) {
		if (sym->st_name) 
			printf("%x %s\t0x%x\n", sym->st_info, (char*) (sym->st_name + (uint32_t)strtab_d), sym->st_value);
		sym++;
	}

	if (reltab) {
		printf("Found relocation table\n");
		printf("Link: %x\tInfo %x\n", reltab->sh_link, reltab->sh_info);

		elf32_rel* r 	= ((uint32_t) data) + reltab->sh_offset;
		elf32_rel* last = ((uint32_t) r) + reltab->sh_size;
		
		elf32_shdr* target = ((uint32_t) data + ehdr->e_shoff) + (reltab->sh_info * ehdr->e_shentsize);


		while(r < last) {

			uint8_t t 	= (unsigned char) (r->r_info);
			uint8_t s 	= (r->r_info) >> 8;
			sym 		= (((uint32_t) data) + symtab->sh_offset);
			sym += s;
			
			char* sym_name = (char*) (sym->st_name + (uint32_t)strtab_d);
	

			uint32_t addend = *(uint32_t*) ((uint32_t) data + target->sh_offset + r->r_offset);
			addend -= r->r_offset;
			addend += 0;
			printf("addr %x addend %x type %X %s\n", r->r_offset, addend, t, sym_name);
			//printf("value @ offset: %x %x\n", addend, addend - r->r_offset);
			r++;
		}
	}	
	
}



struct elf_executable {
	uint32_t* pd;	/* page directory */
	struct _proc_mmap* m;
	elf32_ehdr* ehdr;
	uint32_t esp;
	uint32_t eip;
	void (*execute)(struct elf_executable*);
};

extern void enter_usermode(uint32_t eip, uint32_t esp);

void elf_load(int i_no) {
	inode* ein = ext2_inode(1, i_no);
	
	uint32_t* data = ext2_open(ein);
	elf32_ehdr * ehdr = (elf32_ehdr*) data; 
	assert(ehdr->e_ident[0] == ELF_MAGIC);

	//elf_objdump(data);
	elf32_phdr* phdr 		= (uint32_t) data + ehdr->e_phoff;
	elf32_phdr* last_phdr 	= (uint32_t) phdr + (ehdr->e_phentsize * ehdr->e_phnum);
	uint32_t off = (phdr->p_vaddr - phdr->p_paddr);

	extern uint32_t* KERNEL_PAGE_DIRECTORY;
	/* Make a new page-directory and swap into it */
	uint32_t* elf_pd = k_create_pagedir(0x0, 0, 0x7);	
	//k_copy_pagedir(elf_pd, KERNEL_PAGE_DIRECTORY);
	k_map_kernel(elf_pd);
	/* Map the elf data into the new page directory */
	// for (int i = 0; i <= ein->size; i += 0x1000) {
	// 	_paging_map(elf_pd, k_virt_to_phys((uint32_t)data + i), (uint32_t)data + i, 0x3 );
	// }
	
	k_swap_pd(elf_pd);


	while(phdr < last_phdr) {
		printf("LOAD:\toff 0x%x\tvaddr\t0x%x\tpaddr\t0x%x\n\t\tfilesz\t%d\tmemsz\t%d\talign\t%d\t\n",
		 	phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz, phdr->p_memsz, phdr->p_align);
		
		for (int i = 0; i <= phdr->p_memsz; i += 0x1000) {
			uint32_t phys = k_page_alloc();					// Allocate a physical page
			k_paging_map(phys, phdr->p_vaddr + i, 0x7);			// Map that page in KPD
			//printf("Mapped %x->%x\n", phys, phdr->p_vaddr + i);
		}
		memset(phdr->p_vaddr, 0, phdr->p_memsz);
		memcpy(phdr->p_vaddr, (uint32_t)data + phdr->p_offset, phdr->p_memsz);
	
		phdr++;
	}
	last_phdr--;
	uint32_t base = (last_phdr->p_vaddr + last_phdr->p_memsz + 0x1000) & ~0xFFF;

	cp_mmap.pd = elf_pd;
	cp_mmap.base = base;
	cp_mmap.brk = base;

	k_paging_map(k_page_alloc(), base, 0x7);

	printf("Program entry @ %#x\n", ehdr->e_entry);

	uint32_t* stack = 0x4F00;
	k_paging_map(k_page_alloc(), 0x4000, 0x7);
	char* var1 = 0x4000;
	char** v = 0x4500;
	*v = var1;
	strcpy(var1, "new.s");

	*--stack = var1;	// argv
	*--stack = 0;	// argc

	enter_usermode(ehdr->e_entry, stack);
	for(;;);
	free(data);
	printf("Returned from program\n");
	extern uint32_t* KERNEL_PAGE_DIRECTORY;
	k_swap_pd(KERNEL_PAGE_DIRECTORY);

	free_pagedir(elf_pd);

}


void elf_execute(const char* path) {
	elf_load(pathize(path));
}