#include "loader/elf/elf.h"
#include "loader/elf/elfloader.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "fs/file.h"

#include "stdint.h"

/*
|--------------------------------------------------------------------------
| ELF Validation
|--------------------------------------------------------------------------
*/

bool elf_is_valid(void* elf_header) {
    if (!elf_header) return false;
    unsigned char* ident = (unsigned char*)elf_header;
    return ident[0] == ELFMAG0 &&
           ident[1] == ELFMAG1 &&
           ident[2] == ELFMAG2 &&
           ident[3] == ELFMAG3;
}

bool elf_is_32bit(void* elf_header) {
    if (!elf_header) return false;
    unsigned char* ident = (unsigned char*)elf_header;
    return ident[EI_CLASS] == ELFCLASS32;
}

bool elf_is_64bit(void* elf_header) {
    if (!elf_header) return false;
    unsigned char* ident = (unsigned char*)elf_header;
    return ident[EI_CLASS] == ELFCLASS64;
}

/*
|--------------------------------------------------------------------------
| ELF Entry
|--------------------------------------------------------------------------
*/

void* elf32_get_entry_ptr(struct elf32_header* header) {
    if (!header) return NULL;
    return (void*)(uintptr_t)header->e_entry;
}

void* elf64_get_entry_ptr(struct elf64_header* header) {
    if (!header) return NULL;
    return (void*)(uintptr_t)header->e_entry;
}

elf32_addr elf32_get_entry(struct elf32_header* header) {
    if (!header) return 0;
    return header->e_entry;
}

elf64_addr elf64_get_entry(struct elf64_header* header) {
    if (!header) return 0;
    return header->e_entry;
}

/*
|--------------------------------------------------------------------------
| Program / Section Header Access
|--------------------------------------------------------------------------
*/

struct elf32_phdr* elf32_get_phdr(struct elf32_header* header, uint16_t index) {
    if (!header || index >= header->e_phnum) return NULL;
    return (struct elf32_phdr*)((uintptr_t)header + header->e_phoff + index * header->e_phentsize);
}

struct elf64_phdr* elf64_get_phdr(struct elf64_header* header, uint16_t index) {
    if (!header || index >= header->e_phnum) return NULL;
    return (struct elf64_phdr*)((uintptr_t)header + header->e_phoff + index * header->e_phentsize);
}

struct elf32_shdr* elf32_get_shdr(struct elf32_header* header, uint16_t index) {
    if (!header || index >= header->e_shnum) return NULL;
    return (struct elf32_shdr*)((uintptr_t)header + header->e_shoff + index * header->e_shentsize);
}

struct elf64_shdr* elf64_get_shdr(struct elf64_header* header, uint16_t index) {
    if (!header || index >= header->e_shnum) return NULL;
    return (struct elf64_shdr*)((uintptr_t)header + header->e_shoff + index * header->e_shentsize);
}

/*
|--------------------------------------------------------------------------
| Dynamic Section Access
|--------------------------------------------------------------------------
*/

struct elf32_dyn* elf32_get_dynamic(struct elf32_header* header, struct elf32_phdr* phdr) {
    if (!header || !phdr || phdr->p_type != PT_DYNAMIC) return NULL;
    return (struct elf32_dyn*)((uintptr_t)header + phdr->p_offset);
}

struct elf64_dyn* elf64_get_dynamic(struct elf64_header* header, struct elf64_phdr* phdr) {
    if (!header || !phdr || phdr->p_type != PT_DYNAMIC) return NULL;
    return (struct elf64_dyn*)((uintptr_t)header + phdr->p_offset);
}
