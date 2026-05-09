;--------------------------------------
; ViOS Bootloader
;--------------------------------------
; Assembles to 512 bytes, loads kernel, switches to protected mode
;--------------------------------------
%define CODE_SEG 0x08
%define DATA_SEG 0x10
%ifndef KERNEL_LBA_START
%define KERNEL_LBA_START 400
%endif
%ifndef KERNEL_SECTOR_COUNT
%define KERNEL_SECTOR_COUNT 0x1000
%endif

[BITS 16]
ORG 0x7C00

jmp start
nop

;-----------------------------
; FAT16 / Boot record header (decimal for high bytes)
;-----------------------------
OEMIdentifier       db "ViOSKERN"
BytesPerSector      dw 512
SectorsPerCluster   db 8
ReservedSectors     dw 200
FATCopies           db 2
RootDirEntries      dw 512
NumSectors          dw 0
MediaType           db 248         ; 0xF8
SectorsPerFAT       dw 256         ; 0x100
SectorsPerTrack     dw 32
NumberOfHeads       dw 64
HiddenSectors       dd 0
SectorsBig          dd 7755188      ; 0x773594

DriveNumber         db 128         ; 0x80
WinNTBits           db 0
Signature           db 41          ; 0x29
VolumeID            dd 0xD105
VolumeIDString      db "ViOS boot  "
FileSystemType      db "FAT16   "

;-----------------------------
; Bootloader entry
;-----------------------------
start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

;-----------------------------
; Load GDT and switch to protected mode
;-----------------------------
load_protected:
    cli
    lgdt [gdt_descriptor]        ; load GDT
    mov eax, cr0
    or eax, 1                    ; enable protected mode
    mov cr0, eax
    jmp CODE_SEG:load32          ; far jump to 32-bit loader

;-----------------------------
; GDT (Global Descriptor Table)
;-----------------------------
gdt_start:
gdt_null:      dd 0, 0

gdt_code:      dw 0xFFFF
               dw 0
               db 0x00
               db 0x9A
               db 0xCF
               db 0x00

gdt_data:      dw 0xFFFF
               dw 0
               db 0x00
               db 0x92
               db 0xCF
               db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

;-----------------------------
; 32-bit loader entry point
;-----------------------------
[BITS 32]
load32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000             ; 32-bit stack

    ; Enable A20 line
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Load kernel from disk (LBA)
    mov eax, KERNEL_LBA_START
    mov ecx, KERNEL_SECTOR_COUNT
    mov edi, 0x0100000
    call ata_lba_read

    ; Jump to loaded kernel
    jmp CODE_SEG:0x0100000

;-----------------------------
; ATA LBA Sector Read
;-----------------------------
ata_lba_read:
    push ebp
    push esi
    mov esi, eax
    mov ebp, ecx

.next_sector:
    test ebp, ebp
    jz .done

    mov eax, esi
    shr eax, 24
    and al, 0x0F
    or al, 0xE0
    mov dx, 0x1F6
    out dx, al

    mov al, 1
    mov dx, 0x1F2
    out dx, al

    mov eax, esi
    mov dx, 0x1F3
    out dx, al

    mov dx, 0x1F4
    shr eax, 8
    out dx, al

    mov dx, 0x1F5
    shr eax, 8
    out dx, al

    mov al, 32
    mov dx, 0x1F7
    out dx, al

.wait_ready:
    in al, dx
    test al, 0x80
    jnz .wait_ready
    test al, 0x08
    jz .wait_ready

    mov ecx, 256
    mov dx, 0x1F0
    cld
    rep insw

    inc esi
    dec ebp
    jmp .next_sector

.done:
    pop esi
    pop ebp
    ret

;-----------------------------
; Pad to 512 bytes (boot sector)
;-----------------------------
times 510-($-$$) db 0
dw 0xAA55
