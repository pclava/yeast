#ifndef MOF_H
#define MOF_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MIPS OBJECT FILE
 *
 * Minimal executable and linkable format for this assembler, linker, and emulator toolchain
 * File is stored little-endian
 *
 * HEADER (32 bytes)
 * TEXT
 * DATA
 * RELOCATION DATA
 * SYMBOL TABLE
 * STRING TABLE
 */

enum mof_segment {
    TEXT,
    DATA,
    BSS,
    KTEXT,
    KDATA,
    UNDEF
};

enum mof_binding {
    LOCAL,
    GLOBAL
};

enum mof_reloctype {
    R_32,
    R_26,
    R_PC16,
    R_HI16,
    R_LO16
};

struct mof_header {
    uint32_t magic; // must equal MOF_MAGIC
    uint32_t text;  // size in bytes of text segment
    uint32_t data;  // size in bytes of data segment
    uint32_t bss;   // size in bytes of bss segment
    uint32_t ktext; // size in bytes of kernel text segment
    uint32_t kdata; // size in bytes of kernel data segment
    uint32_t rels;  // size in bytes of relocation table
    uint32_t syms;  // size in bytes of symbol table
    uint32_t entry; // program entry address
};

struct mof_file {
    struct mof_header hdr;          // header
    uint32_t size;                  // size in bytes of file
    void *file;                     // memory mapped file

    uint32_t *text;                 // pointer to text segment
    uint8_t *data;                  // pointer to data
    uint32_t *ktext;                // pointer to kernel text
    uint8_t *kdata;                 // pointer to kernel data
    struct mof_relocation *relocs;  // pointer to relocation data
    struct mof_symbol *syms;        // pointer to symbol table
    char *strings;                  // pointer to string table
};

#define MOF_MAGIC 0x00464f4d; // "MOF" little-endian
#define MOF_HEADERSIZE 36
#define MOF_TXTOFF (MOF_HEADERSIZE)
#define MOF_DTAOFF(head) (MOF_TXTOFF + (head)->text)
#define MOF_KTXTOFF(head) (MOF_DTAOFF(head) + (head)->data)
#define MOF_KDTAOFF(head) (MOF_KTXTOFF(head) + (head)->ktext)
#define MOF_RELOFF(head) (MOF_KDTAOFF(head) + (head)->kdata)
#define MOF_SYMOFF(head) (MOF_RELOFF(head) + (head)->rels)
#define MOF_STROFF(head) (MOF_SYMOFF(head) + (head)->syms)

struct mof_symbol {
    uint32_t index;     // offset in string table
    uint32_t offset;    // offset of symbol from start of segment
    uint16_t segment;   // segment of symbol (text or data or bss)
    uint16_t binding;   // symbol binding (local or global)
};

#define MOF_SYMSIZE 12

struct mof_relocation {
    // "address at (segment+target_offset) requires relocation of type (reloc_type) for the symbol (&strings[index])"
    uint32_t index;      // offset in string table of dependency
    uint32_t offset;     // offset of target from start of segment
    uint16_t segment;    // segment of target (text or data). who needs the relocation?
    uint16_t type;       // type of relocation
};

#define MOF_RELOCSIZE 12

// METHODS

// Returns whether header represents valid mof file
int mof_is_valid(const struct mof_header *hdr);

// Section access return pointer to start of segment
uint32_t *mof_text(void *file);
uint8_t *mof_data(void *file, const struct mof_header *hdr);
uint32_t *mof_ktext(void *file, const struct mof_header *hdr);
uint8_t *mof_kdata(void *file, const struct mof_header *hdr);
struct mof_relocation *mof_relocs(void *file, const struct mof_header *hdr);
struct mof_symbol *mof_symbols(void *file, const struct mof_header *hdr);
char *mof_strtab(void *file, const struct mof_header *hdr);

// Get pointer to string at given index in string table
const char *mof_get_str(void *file, const struct mof_header *hdr, uint32_t index);
//
// // Reads a header from the file. Does not check validity
// int mof_read_header(FILE *file, struct mof_header *hdr);
//
// // Writes a header to the file. Writes the magic number
// int mof_write_header(FILE *file, struct mof_header *hdr);
//
// // Write relocation entry or symbol to file
// int mof_write_relocation(FILE *file, const struct mof_relocation *reloc);
// int mof_write_symbol(FILE *file, const struct mof_symbol *sym);

#ifdef __cplusplus
}
#endif

#endif //MOF_H