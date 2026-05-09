ORG 0x7C00
BITS 16

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
KERNEL_LBA_START equ 400
; ATA sector count register is 8-bit (1..255, with 0 meaning 256).
; Default can be overridden by build system.
%ifndef KERNEL_SECTOR_COUNT
KERNEL_SECTOR_COUNT equ 0x1000
%endif

jmp short start
nop

; FAT16 Header
OEMIdentifier       db "ViOSKERN"
BytesPerSector      dw 0x200
SectorsPerCluster   db 0x80
ReservedSectors     dw 200
FATCopies           db 2
RootDirEntries      dw 512
NumSectors          dw 0
MediaType           db 0xF8
SectorsPerFAT       dw 0x100
SectorsPerTrack     dw 0x20
NumberOfHeads       dw 0x40
HiddenSectors       dd 0
SectorsBig          dd 0x773594

; Extended BPB (Dos 4.0)
DriveNumber         db 0x80
WinNTBits           db 0
Signature           db 0x29
VolumeID            dd 0xD105
VolumeIDString      db "ViOS boot  "
FileSystemType      db "FAT16   "

start:
    jmp 0:step2

step2:
    cli

    mov ax, 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov es, ax

    mov sp, 0x7C00

    sti

.load_protected:
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:load32

        
; GDT
gdt_start:
gdt_null:
    dd 0
    dd 0

gdt_code:
    dw 0xFFFF
    dw 0
    db 0
    db 0x9A
    db 11001111b
    db 0

gdt_data:
    dw 0xFFFF
    dw 0
    db 0
    db 0x92
    db 11001111b
    db 0

gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
    
[BITS 32]
load32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    in al, 0x92
    or al, 0x02
    out 0x92, al

    mov eax, KERNEL_LBA_START
    mov ecx, KERNEL_SECTOR_COUNT
    mov edi, 0x0100000
    call ata_lba_read
    jmp CODE_SEG:0x0100000

ata_lba_read:
    mov ebx, eax
    shl eax, 24
    or eax, 0xE0
    mov dx, 0x1F6
    out dx, al
    
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al

    mov eax, ebx
    mov dx, 0x1F3
    out dx, al

    mov dx, 0x1F4
    mov eax, ebx
    shr eax, 8
    out dx, al

    mov dx, 0x1F5
    mov eax, ebx
    shr eax, 16
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

.next_sector:
    push ecx

.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .try_again

    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next_sector

    ret

times 510-($-$$) db 0
dw 0xAA55
