#include "user.h"
#include "file.h"
#include "tty.h"

int main(void) {
    // Open stdin stdout stderr
    if (open("terminal", O_RDWR) == -1) {
        // terminal node doesn't exist
        mknod("terminal", TERM_MAJ);
        open("terminal", O_RDWR);
    }
    dup(0); // stdout
    dup(0); // stderr
    printf("init: terminal opened\n");

    while (1) {
        printf("init: starting sh\n\r");
        int shpid = fork();
        if (shpid < 0) {
            printf("no fork :(\n\r");
            exit(1); // i've never tested what happens if there are no processes but i think its ok
        }
        if (shpid == 0) {
            char *argv[] = {"sh", NULL};
            exec("sh", argv);
            exit(1);
        }

        while (1) {
            unsigned int status;
            unsigned char child = wait(&status);

            // Restart shell
            if (child == shpid) {
                printf("init: sh closed with code %u, restarting\n\r", status);
                break;
            }

            // If it wasn't the shell, it was a childless process
        }
    }
}
