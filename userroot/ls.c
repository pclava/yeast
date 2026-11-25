#include "file.h"
#include "user.h"

// TODO: stat syscall to ensure path is a directory
int main(int argc, char *argv[]) {
    char buf[128];
    char *path;
    if (argc == 1) {
        getcwd(buf);
        path = buf;
    }
    else if (argc == 2) {
        path = argv[1];
    }
    else {
        printf("Usage: ls file\n\r");
        return 1;
    }

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("ls: path %s not found\n\r", argv[1]);
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        printf("ls: could not read file\n\r");
        return 1;
    }
    if (st.type != F_DIRECTORY) {
        printf("ls: %s is not a directory\n\r", path);
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
    return 0;
}