/**
 * @file elf.h
 * @brief Full ELF32/ELF64 definitions and structures.
 */
#ifndef ELF_H
#define ELF_H

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

/*
|--------------------------------------------------------------------------
| ELF Identification
|--------------------------------------------------------------------------
*/

#define EI_NIDENT   16

#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_ABIVERSION 8

#define ELFMAG0     0x7F
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

#define ELFDATANONE  0
#define ELFDATA2LSB  1
#define ELFDATA2MSB  2

#define EV_NONE      0
#define EV_CURRENT   1

#define ELFOSABI_NONE       0
#define ELFOSABI_SYSV       0
#define ELFOSABI_LINUX      3

/*
|--------------------------------------------------------------------------
| ELF Types
|--------------------------------------------------------------------------
*/

typedef uint32_t elf32_addr;
typedef uint32_t elf32_off;
typedef uint16_t elf32_half;
typedef uint32_t elf32_word;
typedef int32_t  elf32_sword;

typedef uint64_t elf64_addr;
typedef uint64_t elf64_off;
typedef uint16_t elf64_half;
typedef uint32_t elf64_word;
typedef int32_t  elf64_sword;
typedef uint64_t elf64_xword;
typedef int64_t  elf64_sxword;

/*
|--------------------------------------------------------------------------
| ELF Header
|--------------------------------------------------------------------------
*/

struct elf32_header {
    unsigned char e_ident[EI_NIDENT];
    elf32_half e_type;
    elf32_half e_machine;
    elf32_word e_version;
    elf32_addr e_entry;
    elf32_off  e_phoff;
    elf32_off  e_shoff;
    elf32_word e_flags;
    elf32_half e_ehsize;
    elf32_half e_phentsize;
    elf32_half e_phnum;
    elf32_half e_shentsize;
    elf32_half e_shnum;
    elf32_half e_shstrndx;
};

struct elf64_header {
    unsigned char e_ident[EI_NIDENT];
    elf64_half e_type;
    elf64_half e_machine;
    elf64_word e_version;
    elf64_addr e_entry;
    elf64_off  e_phoff;
    elf64_off  e_shoff;
    elf64_word e_flags;
    elf64_half e_ehsize;
    elf64_half e_phentsize;
    elf64_half e_phnum;
    elf64_half e_shentsize;
    elf64_half e_shnum;
    elf64_half e_shstrndx;
};

/*
|--------------------------------------------------------------------------
| Program Header
|--------------------------------------------------------------------------
*/

#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6
#define PT_TLS     7

#define PF_X 1
#define PF_W 2
#define PF_R 4

struct elf32_phdr {
    elf32_word p_type;
    elf32_off  p_offset;
    elf32_addr p_vaddr;
    elf32_addr p_paddr;
    elf32_word p_filesz;
    elf32_word p_memsz;
    elf32_word p_flags;
    elf32_word p_align;
};

struct elf64_phdr {
    elf64_word  p_type;
    elf64_word  p_flags;
    elf64_off   p_offset;
    elf64_addr  p_vaddr;
    elf64_addr  p_paddr;
    elf64_xword p_filesz;
    elf64_xword p_memsz;
    elf64_xword p_align;
};

/*
|--------------------------------------------------------------------------
| Section Header
|--------------------------------------------------------------------------
*/

#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_HASH     5
#define SHT_DYNAMIC  6
#define SHT_NOTE     7
#define SHT_NOBITS   8
#define SHT_REL      9
#define SHT_SHLIB    10
#define SHT_DYNSYM   11

#define SHF_WRITE     0x1
#define SHF_ALLOC     0x2
#define SHF_EXECINSTR 0x4

struct elf32_shdr {
    elf32_word sh_name;
    elf32_word sh_type;
    elf32_word sh_flags;
    elf32_addr sh_addr;
    elf32_off  sh_offset;
    elf32_word sh_size;
    elf32_word sh_link;
    elf32_word sh_info;
    elf32_word sh_addralign;
    elf32_word sh_entsize;
};

struct elf64_shdr {
    elf64_word  sh_name;
    elf64_word  sh_type;
    elf64_xword sh_flags;
    elf64_addr  sh_addr;
    elf64_off   sh_offset;
    elf64_xword sh_size;
    elf64_word  sh_link;
    elf64_word  sh_info;
    elf64_xword sh_addralign;
    elf64_xword sh_entsize;
};

/*
|--------------------------------------------------------------------------
| Symbol Table
|--------------------------------------------------------------------------
*/

struct elf32_sym {
    elf32_word st_name;
    elf32_addr st_value;
    elf32_word st_size;
    unsigned char st_info;
    unsigned char st_other;
    elf32_half st_shndx;
};

struct elf64_sym {
    elf64_word  st_name;
    unsigned char st_info;
    unsigned char st_other;
    elf64_half  st_shndx;
    elf64_addr  st_value;
    elf64_xword st_size;
};

/* Symbol info macros */
#define ELF32_ST_BIND(i)    ((i)>>4)
#define ELF32_ST_TYPE(i)    ((i)&0xf)
#define ELF32_ST_INFO(b,t)  (((b)<<4)+((t)&0xf))

#define ELF64_ST_BIND(i)    ((i)>>4)
#define ELF64_ST_TYPE(i)    ((i)&0xf)
#define ELF64_ST_INFO(b,t)  (((b)<<4)+((t)&0xf))

/*
|--------------------------------------------------------------------------
| Relocation
|--------------------------------------------------------------------------
*/

struct elf32_rel {
    elf32_addr r_offset;
    elf32_word r_info;
};

struct elf32_rela {
    elf32_addr r_offset;
    elf32_word r_info;
    elf32_sword r_addend;
};

struct elf64_rel {
    elf64_addr r_offset;
    elf64_xword r_info;
};

struct elf64_rela {
    elf64_addr r_offset;
    elf64_xword r_info;
    elf64_sxword r_addend;
};

/* Relocation info macros */
#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s,t) (((s)<<8)+(unsigned char)(t))

#define ELF64_R_SYM(i) ((i)>>32)
#define ELF64_R_TYPE(i) ((i)&0xffffffff)
#define ELF64_R_INFO(s,t) (((s)<<32)+(t))

/*
|--------------------------------------------------------------------------
| Dynamic Section
|--------------------------------------------------------------------------
*/

#define DT_NULL    0
#define DT_NEEDED  1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_SYMENT   11
#define DT_INIT     12
#define DT_FINI     13
#define DT_SONAME   14
#define DT_RPATH    15
#define DT_SYMBOLIC 16
#define DT_PLTREL   20
#define DT_DEBUG    21
#define DT_TEXTREL  22
#define DT_JMPREL   23

struct elf32_dyn {
    elf32_sword d_tag;
    union {
        elf32_word d_val;
        elf32_addr d_ptr;
    } d_un;
};

struct elf64_dyn {
    elf64_sxword d_tag;
    union {
        elf64_xword d_val;
        elf64_addr  d_ptr;
    } d_un;
};

/*
|--------------------------------------------------------------------------
| Function Prototypes
|--------------------------------------------------------------------------
*/

bool elf_is_valid(void* elf_header);
bool elf_is_32bit(void* elf_header);
bool elf_is_64bit(void* elf_header);

void* elf32_get_entry_ptr(struct elf32_header* header);
void* elf64_get_entry_ptr(struct elf64_header* header);
elf32_addr elf32_get_entry(struct elf32_header* header);
elf64_addr elf64_get_entry(struct elf64_header* header);

struct elf32_phdr* elf32_get_phdr(struct elf32_header* header, uint16_t index);
struct elf64_phdr* elf64_get_phdr(struct elf64_header* header, uint16_t index);

struct elf32_shdr* elf32_get_shdr(struct elf32_header* header, uint16_t index);
struct elf64_shdr* elf64_get_shdr(struct elf64_header* header, uint16_t index);

struct elf32_dyn* elf32_get_dynamic(struct elf32_header* header, struct elf32_phdr* phdr);
struct elf64_dyn* elf64_get_dynamic(struct elf64_header* header, struct elf64_phdr* phdr);

#endif /* ELF_H */