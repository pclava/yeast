
#include "file.h"
#include "user.h"
#include "fs.h"

void intro();
void colors();
void yeast();
void ls();

int main() {

    // open stdin,out,err
    int fd0 = open("terminal", O_RDWR | O_CREAT);
    int fd1 = dup(0);
    int fd2 = dup(0);

    intro();

    char buf[128];
    while (1) {
        printf("%% ");
        gets(buf, sizeof(buf));
        unsigned int n = strlen(buf);

        if (strcmp(buf, "panic") == 0) {
            *(int *)n = 1;
        }
        else if (strcmp(buf, "colors") == 0) {
            colors();
        }
        else if (strcmp(buf, "clear") == 0) {
            termctl(TTY_CLEAR);
        }
        else if (strcmp(buf, "intro") == 0) {
            termctl(TTY_CLEAR);
            intro();
        }
        else if (strcmp(buf, "yeast") == 0) {
            yeast();
        }
        else if (strcmp(buf, "ls") == 0) {
            ls();
        }
        else if (buf[0] == '\0') {
            continue;
        }
        else {
            printf(ANSI_COLOR_BRIGHT_RED "unrecognized command\n\r" ANSI_COLOR_RESET);
        }

        termctl(TTY_FLUSH);
    }
}

void yeast() {
    printf(ANSI_COLOR_YELLOW KERNEL_NAME ANSI_COLOR_RESET " kernel " KERNEL_VERS " by paul clavaud\n\r");
}

void colors() {
    printf("The ANSI color spectrum:\n\r");
    printf(ANSI_COLOR_BLACK "Black" ANSI_COLOR_RED "Red" ANSI_COLOR_GREEN "Green" ANSI_COLOR_YELLOW "Yellow" );
    printf(ANSI_COLOR_BLUE "Blue" ANSI_COLOR_MAGENTA "Magenta" ANSI_COLOR_CYAN "Cyan" ANSI_COLOR_WHITE "White\n\r" );
    printf(ANSI_COLOR_BRIGHT_BLACK "BrightBlack" ANSI_COLOR_BRIGHT_RED "BrightRed" ANSI_COLOR_BRIGHT_GREEN "BrightGreen" ANSI_COLOR_BRIGHT_YELLOW "BrightYellow\n\r" );
    printf(ANSI_COLOR_BRIGHT_BLUE "BrightBlue" ANSI_COLOR_BRIGHT_MAGENTA "BrightMagenta" ANSI_COLOR_BRIGHT_CYAN "BrightCyan" ANSI_COLOR_BRIGHT_WHITE "BrightWhite\n\r" );
    printf(ANSI_COLOR_BLACK);

    printf(ANSI_BG_BLACK "Black" ANSI_BG_RED "Red" ANSI_BG_GREEN "Green" ANSI_BG_YELLOW "Yellow" );
    printf(ANSI_BG_BLUE "Blue" ANSI_BG_MAGENTA "Magenta" ANSI_BG_CYAN "Cyan" ANSI_BG_WHITE "White\n\r" );
    printf(ANSI_BG_BRIGHT_BLACK "BrightBlack" ANSI_BG_BRIGHT_RED "BrightRed" ANSI_BG_BRIGHT_GREEN "BrightGreen" ANSI_BG_BRIGHT_YELLOW "BrightYellow\n\r" );
    printf(ANSI_BG_BRIGHT_BLUE "BrightBlue" ANSI_BG_BRIGHT_MAGENTA "BrightMagenta" ANSI_BG_BRIGHT_CYAN "BrightCyan" ANSI_BG_BRIGHT_WHITE "BrightWhite\n\r" );

    printf(ANSI_COLOR_RESET);
}

void intro() {
    printf("\n\r" ANSI_BG_YELLOW ANSI_COLOR_BLACK);
    for (int i = 0; i < 31; i++) {
        printf(" ");
    }
    printf(ANSI_COLOR_BRIGHT_WHITE ANSI_BG_BLACK);
    printf(" welcome to " ANSI_COLOR_YELLOW "yeast ");
    printf(ANSI_COLOR_BLACK ANSI_BG_YELLOW);
    for (int i = 49; i < VGA_WIDTH; i++) {
        printf(" ");
    }
    printf(ANSI_COLOR_RESET "\n\r");
}

void ls() {
    int fd = open("/", O_RDONLY);

    struct direntry de;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inode == 0) continue;
        printf("%s\n\r", de.name);
    }

    close(fd);
}