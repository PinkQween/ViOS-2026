#ifndef ELFLOADER_H
#define ELFLOADER_H

#include "status.h"
#include <stddef.h>
#include <stdint.h>

#define ELFCLASS32 1
#define ELFCLASS64 2

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_SONAME   14
#define DT_RPATH    15
#define DT_SYMBOLIC 16
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_PLTREL   20
#define DT_DEBUG    21
#define DT_TEXTREL  22
#define DT_JMPREL   23

#define ELF_MAX_NEEDED_LIBS 256
#define ELF_MAX_SEGMENTS 32

struct elf_segment {
    void* vaddr;
    void* paddr;
    size_t memsz;
    size_t filesz;
    uint32_t flags;
};

struct elf_file {
    char filename[128];
    void* elf_memory; // Raw file data
    size_t in_memory_size;
    int elf_class;

    struct elf_segment segments[ELF_MAX_SEGMENTS];
    int segment_count;
    void* base_address; // Base virtual address of first LOAD segment

    void* entry;
    
    // Dynamic section data
    void* dynsym;
    char* dynstr;
    void* rel;
    size_t rel_size;
    void* rela;
    size_t rela_size;
    void* jmprel;
    size_t jmprel_size;
    void* hash;
    void* pltgot;

    struct elf_file* needed_libraries[ELF_MAX_NEEDED_LIBS];
    int needed_count;
};

// Allocation helpers
struct elf_file* elf_file_new();
void elf_file_free(struct elf_file* file);

// ELF loading
status_t elf_load(const char* filename, struct elf_file** file_out);
void elf_close(struct elf_file* file);

// Dynamic linking
status_t elf_load_needed_libraries(struct elf_file* file);
status_t elf_apply_relocations(struct elf_file* file);

// Symbols
status_t elf_lookup_symbol(struct elf_file* file, const char* symbol_name, void** out_addr);

// Headers & execution
void* elf_header(struct elf_file* file);
struct elf32_header* elf32_header(struct elf_file* file);
struct elf64_header* elf64_header(struct elf_file* file);

void* elf_entry_point(struct elf_file* file);

#endif // ELFLOADER_H