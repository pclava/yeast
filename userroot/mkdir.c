#include "user.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: mkdir <dir>\n");
        return 1;
    }

    if (mkdir(argv[1]) == -1) {
        printf("mkdir failed\n");
        return 1;
    }

    return 0;
}