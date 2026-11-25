#include "file.h"
#include "user.h"

// TODO: stat syscall to ensure path is a directory
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ls file\n\r");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        printf("ls: path %s not found\n\r", argv[1]);
        return 1;
    }

    printf("inode: name\n\r");
    printf("-----------\n\r");
    struct direntry de;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inode == 0) continue;
        printf("%d: %s\n\r", de.inode, de.name);
    }

    close(fd);
}