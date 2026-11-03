#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/fs.h"

typedef unsigned int uint;

#define FSSIZE 4096 // file system size in blocks
#define NINODES 200
#define INODESECT i

#define min(a, b) ((a) < (b) ? (a) : (b))

int nbitmap = 1 + FSSIZE / BITS_PER_BLOCK;
int ninodes = 1 + NINODES / INODES_PER_BLOCK;
int nlog = 0;
int nmeta;
int ndata;

uint freeinode = 0; // next free inode
uint freeblock = 0; // next free block
char zeroblock[BLOCKSIZE] = {0};
struct superblock sb;
FILE *diskimg;
char *diskname;
uint i_rootdir;

// stolen from xv6
uint xint(uint x)
{
    uint y;
    uint8_t *a = (uint8_t *)&y;
    a[0] = x;
    a[1] = x >> 8;
    a[2] = x >> 16;
    a[3] = x >> 24;
    return y;
}

// Expects a sector-sized buffer, and writes it to the given sector
void wsect(uint sector, void *buf) {
    if (fseek(diskimg, sector * BLOCKSIZE, SEEK_SET) != 0) {
        fprintf(stderr, "fseek");
        exit(1);
    }

    if (fwrite(buf, 1, BLOCKSIZE, diskimg) != BLOCKSIZE) {
        fprintf(stderr, "fwrite");
        exit(1);
    }
}

// Opposite of wsect
void rsect(uint sector, void *buf) {
    if (fseek(diskimg, sector * BLOCKSIZE, SEEK_SET) != 0) {
        fprintf(stderr, "fseek");
        exit(1);
    }

    unsigned long x = fread(buf, 1, BLOCKSIZE, diskimg);
    if (x != BLOCKSIZE) {
        if (feof(diskimg)) {
            fprintf(stderr, "End of file reached safely after reading %zu items.\n", x);
        }
        if (ferror(diskimg)) {
            // A hardware or system level error occurred!
            perror("Error reading file");
        }

        fprintf(stderr, "fread %lu", x);
        exit(1);
    }
}

// Writes the inode at the given index
void winode(uint i, struct dinode *dinode) {
    char buf[BLOCKSIZE];

    // Calculate sector
    uint sector = IBLOCK(i, sb);

    // Read existing sector
    rsect(sector, buf);

    // Get offset, turn buf into array of dinodes
    uint offset = i % INODES_PER_BLOCK;
    struct dinode *dbuf = (struct dinode *)buf;

    // Write dinode
    dbuf[offset] = *dinode;

    // Write sector again
    wsect(sector, buf);
}

// Opposite of winode
void rinode(uint i, struct dinode *dinode) {
    char buf[BLOCKSIZE];
    uint sector = IBLOCK(i, sb);
    rsect(sector, buf);
    uint offset = i % INODES_PER_BLOCK;
    struct dinode *dbuf = (struct dinode *)buf;
    *dinode = dbuf[offset];
}

// The main way data is written to disk files
// Takes an inumber and a buffer of data and writes it to the file described by the inode
void write_to_inode(uint i, void *data, size_t bytes) {
    char buf[BLOCKSIZE];
    uint sect;
    char *d = (char *)data;
    i = xint(i);

    // Read inode
    struct dinode dinode;
    rinode(i, &dinode);

    uint boffset = dinode.size;

    // Write data
    while (bytes > 0) {
        // Which block do we need to write to?
        uint file_block = boffset / BLOCKSIZE;
        assert(file_block < MAX_FILE);

        // Get the sector we need to write to
        // Block could be direct or indirect
        if (file_block < NDIRECT) {
            // Get sector, and allocate if it doesn't exist
            if (xint(dinode.data[file_block]) == 0) {
                dinode.data[file_block] = xint(freeblock++);
                printf("Allocated sector %d\n", dinode.data[file_block]);
            }
            sect = xint(dinode.data[file_block]);
        }
        else {
            // Allocate indirect block
            if (xint(dinode.data[NDIRECT]) == 0) {
                dinode.data[NDIRECT] = xint(freeblock++);
            }

            rsect(xint(dinode.data[NDIRECT]), buf);

            memset(buf, 0, BLOCKSIZE);
            uint *indirect = (uint *)buf;
            uint indirect_index = file_block - NDIRECT;

            // Block not allocated
            if (indirect[indirect_index] == 0) {
                indirect[indirect_index] = xint(freeblock++);
                wsect(xint(dinode.data[NDIRECT]), buf);
            }

            sect = xint(indirect[indirect_index]);
        }

        memset(buf, 0, BLOCKSIZE);
        rsect(sect, buf);
        uint bytes_in_block = ((file_block+1) * BLOCKSIZE) - boffset;
        uint to_write = min(bytes, bytes_in_block);
        memcpy(buf + (boffset - (file_block * BLOCKSIZE)), d, to_write);
        wsect(sect, buf);
        bytes -= to_write;
        boffset += to_write;
        d += to_write;
    }

    // Update inode
    dinode.size = boffset;
    winode(i, &dinode);
}

// Creates and initializes the superblock
void create_superblock(void) {
    nmeta = 1 + 1 + ninodes + nlog + nbitmap;
    ndata = FSSIZE - nmeta;

    sb.magic = FSMAGIC;
    sb.size = xint(FSSIZE);
    sb.ndata = xint(ndata);
    sb.ninodes = xint(ninodes);
    sb.nlog = xint(nlog);
    sb.logstart = xint(2);
    sb.inodestart = xint(sb.logstart + sb.nlog);
    sb.bitmapstart = xint(sb.inodestart + sb.ninodes);

    freeblock = nmeta;
}

// Creates and writes a new inode to the disk
uint allocate_inode(uint32_t type) {
    uint i = freeinode++;
    assert(i < NINODES);
    struct dinode dinode = {0};
    dinode.type = xint(type);
    dinode.size = xint(0);
    dinode.nlink = xint(1);
    winode(i, &dinode);
    return i;
}

// Adds a directory entry to a directory inode
// This points to another inode, and has the given name
void add_direntry(uint i_parent, char *name, uint i_child) {
    struct direntry direntry = {0};
    direntry.inode = xint(i_child);
    strncpy(direntry.name, name, DIRSIZE+1);
    write_to_inode(i_parent, &direntry, sizeof(direntry));
}

void fs_init(void ) {
    assert((BLOCKSIZE % sizeof(struct dinode)) == 0);
    char blockbuf[BLOCKSIZE];

    // 1. Create superblock
    create_superblock();
    printf("Created superblock\n");

    // 2. Open and initialize output file
    diskimg = fopen(diskname, "wb+");
    assert(diskimg != NULL);
    printf("Opened disk.img\n");

    // Initialize with zeroes
    for (int i = 0; i < FSSIZE; i += 1) {
        wsect(i, zeroblock);
    }

    // 3. Write superblock
    memset(blockbuf, 0, BLOCKSIZE);
    memcpy(blockbuf, &sb, sizeof(struct superblock));
    wsect(1, blockbuf);
    printf("Wrote superblock\n");

    // 4. Create root directory inode
    i_rootdir = allocate_inode(F_DIRECTORY);
    assert(i_rootdir == 0);
    printf("Created rootdir\n");

    // Add . and ..
    add_direntry(i_rootdir, ".", i_rootdir);
    add_direntry(i_rootdir, "..", i_rootdir);
    printf("Added direntries\n");
}

void add_file(char *filename) {
    FILE *f = fopen(filename, "rb");
    assert(f != NULL);

    // TODO: Check size

    // Get name on its own
    char *name = strrchr(filename, '/');
    if (name == NULL) {
        name = filename;
    } else {
        name++;
    }

    // Client executables are prefixed with _ to avoid confusion
    if (name[0] == '_') name++;

    assert(strlen(name) < DIRSIZE);

    // Create inode
    uint inode = allocate_inode(F_FILE);

    // Add to root directory
    // TODO: generalize
    add_direntry(i_rootdir, name, inode);

    // Write file one sector at a time to the inode
    char buf[BLOCKSIZE];
    uint n;
    while ((n = fread(buf, 1, BLOCKSIZE, f)) > 0) {
        write_to_inode(inode, buf, n);
    }

    fclose(f);
}

// Writes the bitmap after disk init
// Takes advantage of the fact mkfs allocates blocks linearly and contiguously
// And is made easier by the fact we only need one sector for the bitmap
void write_bitmap(uint blocksused) {
    assert(blocksused < BITS_PER_BLOCK);

    unsigned char buf[BLOCKSIZE];
    memset(buf, 0, BLOCKSIZE);

    for (uint i = 0; i < blocksused; i++) {
        buf[i/8] |= (1 << (i % 8));
    }

    wsect(sb.bitmapstart, buf);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "mkfs (name).img files...");
        exit(2);
    }

    diskname = argv[1];

    fs_init();

    // 5. Add files to root
    for (int i = 2; i < argc; i++) {
        add_file(argv[i]);
    }
    printf("Wrote files\n");

    write_bitmap(freeblock);
    printf("Wrote bitmap\n");

    // End
    fclose(diskimg);
    exit(0);

}
