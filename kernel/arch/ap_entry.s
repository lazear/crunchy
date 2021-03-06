[BITS 16]
org 0x8000
global start
start:
	cli
	xor ax, ax 			; Clear AX register
	mov ds, ax			; Set DS-register to 0 
	mov es, ax
	mov fs, ax
	mov gs, ax

	lgdt [gdt_desc] 	; Load the Global Descriptor Table

	mov eax, cr0
	or eax, 1               ; Set bit 0 
	mov cr0, eax

	mov ax, start32
	;jmp $

	jmp 0x8:start32

[BITS 32]

start32:

	mov ax, KERNEL_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov gs, ax

	mov eax, [start-12]			; We need to put _init_pd on the stack
	mov cr3, eax				; set up page directory

	mov eax, cr0
	or eax, 0x80000000			; Enable paging
	mov cr0, eax

	mov esp, [start-4]			; Pass stack
	;mov eax, [start-8]
	call [start-8]				; Jump into entry function

	; Should not reach this point
	;mov eax, 0xDEADBEEF
	jmp $

align 32
gdt:                            ; Address for the GDT

gdt_null:
	dd 0
	dd 0

KERNEL_CODE equ $-gdt		; 0x08
gdt_kernel_code:
	dw 0FFFFh 	; Limit 0xFFFF
	dw 0		; Base 0:15
	db 0		; Base 16:23
	db 09Ah 	; Present, Ring 0, Code, Non-conforming, Readable
	db 0CFh		; Page-granular
	db 0 		; Base 24:31

KERNEL_DATA equ $-gdt
gdt_kernel_data:
	dw 0FFFFh 	; Limit 0xFFFF
	dw 0		; Base 0:15
	db 0		; Base 16:23
	db 092h 	; Present, Ring 0, Code, Non-conforming, Readable
	db 0CFh		; Page-granular
	db 0 		; Base 24:31

gdt_end:                        ; Used to calculate the size of the GDT

gdt_desc:                  ; The GDT descriptor
	dw gdt_end - gdt - 1    ; Limit (size)
	dd gdt                  ; Address of the GDT
