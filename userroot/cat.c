
#include "user.h"
#include "file.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: cat <file>\n\r");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("cat: file %s could not be opened\n\r", argv[1]);
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        printf("cat: could not read file\n\r");
        return 1;
    }
    if (st.type != F_FILE) {
        printf("cat: %s is not a file\n\r", argv[1]);
        return 1;
    }

    char buf[512];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (write(1, buf, n) != n) {
            fprintf(2, "cat: write error\n");
            exit(1);
        }
    }
    printf("\n");

    close(fd);
    return 1;
}
