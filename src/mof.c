#include "mof.h"

int mof_is_valid(const struct mof_header *hdr) {
    return hdr->magic == MOF_MAGIC;
}

uint32_t *mof_text(void *file) {
    return file + MOF_TXTOFF;
}

uint8_t *mof_data(void *file, const struct mof_header *hdr) {
    return file + MOF_DTAOFF(hdr);
}

uint32_t *mof_ktext(void *file, const struct mof_header *hdr) {
    return file + MOF_KTXTOFF(hdr);
}

uint8_t *mof_kdata(void *file, const struct mof_header *hdr) {
    return file + MOF_KDTAOFF(hdr);
}

struct mof_relocation *mof_relocs(void *file, const struct mof_header *hdr) {
    return file + MOF_RELOFF(hdr);
}

struct mof_symbol *mof_symbols(void *file, const struct mof_header *hdr) {
    return file + MOF_SYMOFF(hdr);
}

char *mof_strtab(void *file, const struct mof_header *hdr) {
    return file + MOF_STROFF(hdr);
}

const char *mof_get_str(void *file, const struct mof_header *hdr, const uint32_t index) {
    const char *strtab = mof_strtab(file, hdr);
    return strtab + index;
}
//
// int mof_read_header(FILE *file, struct mof_header *hdr) {
//     return fread(hdr, sizeof(struct mof_header), 1, file) == 1 ? 1 : 0;
// }
//
// int mof_write_header(FILE *file, struct mof_header *hdr) {
//     hdr->magic = MOF_MAGIC;
//     return fwrite(hdr, sizeof(struct mof_header), 1, file) == 1 ? 1 : 0;
// }
//
// int mof_write_relocation(FILE *file, const struct mof_relocation *reloc) {
//     if (fwrite(&reloc->index, sizeof(reloc->index), 1, file) != 1) return 0;
//     if (fwrite(&reloc->offset, sizeof(reloc->offset), 1, file) != 1) return 0;
//     if (fwrite(&reloc->segment, sizeof(reloc->segment), 1, file) != 1) return 0;
//     if (fwrite(&reloc->type, sizeof(reloc->type), 1, file) != 1) return 0;
//     return 1;
// }
//
// int mof_write_symbol(FILE *file, const struct mof_symbol *sym) {
//     if (fwrite(&sym->index, sizeof(sym->index), 1, file) != 1) return 0;
//     if (fwrite(&sym->offset, sizeof(sym->offset), 1, file) != 1) return 0;
//     if (fwrite(&sym->segment, sizeof(sym->segment), 1, file) != 1) return 0;
//     if (fwrite(&sym->binding, sizeof(sym->binding), 1, file) != 1) return 0;
//     return 1;
// }