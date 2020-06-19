#include "vim.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "console.h"
#include "color.h"

static int mode = V_READONLY;

void
help() {
    printf(STDOUT_FILENO, "Usage: vim [FILENAME]\n");
}

char *
readcmd() {
    int len = 0;
//    char *cmd = malloc(CMD_BUF_SZ * sizeof(uchar));
    char *cmd = "";  // TODO: check
    uchar c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != '\n') {
        if (len + 1 > CMD_BUF_SZ) {
            return "CMD_OVERFLOW";
        }
        cmd[len++] = c;
        scrputc(MAX_ROW * MAX_COL + len, c);
    }

    return cmd;
}

static void
clearcli() {
    int i;
    for (i = MAX_COL * MAX_ROW; i < MAX_COL * MAX_ROW + 80; i++) {
        scrputc(i, (' ' & 0xff) | BLACK);
    }
}

int
main(int argc, char *argv[]) {
    if (argc <= 1) {
        help();
        exit();
    }

    // TODO: Restore previous context
    int precursor = getcursor();

    clrscr();
    setcursor(0);
    setconsbuf(ENTER_VIM);

    uchar c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
//        printf(0, "you hit %d", c);
        switch (c) {
            case KEY_UP:
//                printf(0, "you hit right button");
                // TODO: cursor movement
                setcursor(1);
                break;
            case KEY_DN:
//                printf(0, "you hit right button");
                // TODO: cursor movement
                setcursor(1);
                break;
            case KEY_LF:
//                printf(0, "you hit right button");
                // TODO: cursor movement
                setcursor(1);
                break;
            case KEY_RT:
//                printf(0, "you hit right button");
                // TODO: cursor movement
                setcursor(1);
                break;
            case KEY_ESC:
                if (mode == V_INSERT) {
                    mode = V_READONLY;
                    setcursor(CMD_LINE);
                }
                break;
            case ':': {
                if (mode == V_READONLY) {
                    scrputc(MAX_ROW * MAX_COL, ':');
                    char *cmd = readcmd();
                    if (cmd) {
                        // TODO pattern check
                        if (cmd[0] == 'q')
                            goto EXIT;
                    }
                } else if (mode == V_INSERT) {
                    int cp = getcursor();
                    scrputc(cp, c);
                }
                break;
            }
            case 'i': {
                // insert mode
                if (mode == V_READONLY) {
                    mode = V_INSERT;
                } else if (mode == V_INSERT) {
                    int cp = getcursor();
                    scrputc(cp, c);
                }
                break;
            }
            default: {
                if (mode == V_INSERT) {
                    int cp = getcursor();
                    scrputc(cp, c);
                }
                break;
            }
        }
    }

    EXIT:

    setconsbuf(EXIT_VIM);
    clearcli();
    setcursor(precursor);
    exit();

}