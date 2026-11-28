
#include <stddef.h>
#include "user.h"
#include "file.h"

#define BIN "/"
#define NARGS 15
struct cmd {
    int argc; // does not include first argument (exe name)
    char *out; // stdout redirection
    char *in; // stdin redirection
    char *command[1+NARGS+1]; // (path) (args) (NULL)
};

int G = 1;

unsigned int cmdexec(struct cmd *cmd);
unsigned int nativeexec(struct cmd *cmd);

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

int main(void) {

    struct cmd command;
    char buf[128];

    intro();

    while (1) {
        memset(&command, '\0', sizeof(struct cmd));
        printf("%% ");
        gets(buf, sizeof(buf));
        if (*buf == '\0') continue;
        unsigned int n = strlen(buf);

        // 1. Get path
        char *args_start = strchr(buf, ' ');
        if (args_start == NULL) {
            command.command[0] = buf;
            cmdexec(&command);
            continue;
        }

        while (*args_start == ' ') *args_start++ = '\0';
        if (*args_start == '\0') continue;
        if (*buf == '\0') {
            printf("unrecognized command\n\r");
            continue;
        }
        command.command[0] = buf;

        // 2. Parse args
        char *cur = args_start;
        int run = 1;
        int redirect_in = 0;
        int redirect_out = 0;
        while (cur != NULL) {
            char *next = strchr(cur, ' ');
            if (next != NULL) while (*next == ' ') *next++ = '\0';
            // ===

            // Input redirection
            if (strcmp(cur, "<") == 0) {
                if (next == NULL) {
                    printf("error: expected file after <\n");
                    run = 0;
                    break;
                }
                cur = next;
                redirect_in = 1;
                continue;
            }

            // Read file for input redirection
            if (redirect_in) {
                command.in = cur;
                redirect_in = 0;
                cur = next;
                continue;
            }

            // Output redirection
            if (strcmp(cur, ">") == 0) {
                if (next == NULL) {
                    printf("error: expected file after >\n");
                    run = 0;
                    break;
                }
                cur = next;
                redirect_out = 1;
                continue;
            }

            // Read file for output redirecton
            if (redirect_out) {
                command.out = cur;
                redirect_out = 0;
                cur = next;
                // continue;
            }

            // Normal argument
            if (command.argc > NARGS) {
                printf("too many arguments, aborting\n");
                run = 0;
                break;
            }

            // Add to list
            command.command[command.argc+1] = cur;
            command.argc++;

            // Get next
            cur = next;
        }

        // // 3. Execute
        // if (command.in != NULL) {
        //     printf("< %s\n\r", command.in);
        // }
        // if (command.out != NULL) {
        //     printf("> %s\n\r", command.out);
        // }

        if (run) cmdexec(&command);
    }
}

// Commands not associated with a program, executed here
unsigned int nativeexec(struct cmd *cmd) {
    char *cmdname = cmd->command[0];
    if (strcmp(cmdname, "clear") == 0) {
        if (cmd->argc != 0) goto unrecognized;
        termctl(TTY_CLEAR);
        return 0;
    }

    if (strcmp(cmdname, "exit") == 0) {
        if (cmd->argc != 0) goto unrecognized;
        exit(0);
        return 0;
    }

    if (strcmp(cmdname, "yeast") == 0) {
        if (cmd->argc != 0) goto unrecognized;
        printf(ANSI_COLOR_YELLOW KERNEL_NAME ANSI_COLOR_RESET " kernel " KERNEL_VERS " by paul clavaud\n\r");
        return 0;
    }

    if (strcmp(cmdname, "panic") == 0) {
        if (cmd->argc != 0) goto unrecognized;
        *(int *)1 = 1;
        return 0;
    }

    if (strcmp(cmdname, "colors") == 0) {
        if (cmd->argc != 0) goto unrecognized;
        colors();
        return 0;
    }

    if (strcmp(cmdname, "cd") == 0) {
        if (cmd->argc != 1) goto unrecognized;
        if (chdir(cmd->command[1]) == -1) return 1;
        return 0;
    }

    if (strcmp(cmdname, "cwd") == 0) {
        if (cmd->argc != 0) goto unrecognized;
        char buf[128];
        getcwd(buf);
        printf("%s\n\r", buf);
        return 0;
    }

    return -1;
    unrecognized:
    printf("unrecognized command\n\r");
    return 1;
}

unsigned int cmdexec(struct cmd *cmd) {
    char *cmdname = cmd->command[0];
    unsigned int x = nativeexec(cmd);
    if (x != -1) return x;

    char path[128] = BIN;
    memcpy(path+strlen(BIN), cmdname, strlen(cmdname));

    int pid = fork();
    if (pid == 0) {
        exec(path, cmd->command);
        exit(1);
        return 0;
    }

    unsigned int status;
    wait(&status);
    return status;
}