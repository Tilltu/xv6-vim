#include "vim.h"
#include "vimfunc.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "console.h"
#include "color.h"
#include "fcntl.h"

word* addnode(struct word *,int, int,uchar);
word* create_text(int, uchar *);
int readfile(char *, struct text *);
int writefile(char *, struct word *);
int printwords(struct word *, int);

static int mode = V_READONLY;
static text current_text = {NULL, 0, NULL, 0};

void
help() {
    printf(STDOUT_FILENO, "Usage: vim [FILENAME]\n");
}

static int
cmdfilter(int c) {
    // Viewable char: a-z A-Z 0-9 #./\+=-_!@#$%^&*
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '#' || c == '.' ||
        c == '/' || c == '\\' ||
        c == '+' || c == '-' ||
        c == '*' || c == '_' ||
        c == '!' || c == '@' ||
        c == '#' || c == '$' ||
        c == '%' || c == '^' ||
        c == '&')
        return 1;
    else
        return 0;
}

char *
readcmd() {
    int pos;
    int len;
//    printf(STDOUT_FILENO, "cursor pos:%d\n", pos);

    char *cmd = "";
    uchar c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != '\n') {
        pos = getcursor();
        len = (pos - MAX_CHAR) % MAX_COL;
        if (len > CMD_BUF_SZ) {
            printf(STDOUT_FILENO, "len=%d", len);
            return "CMD_OVERFLOW"; // TODO: Better way to handle
        }
        if (cmdfilter(c))
            scrputc(pos, c);
        else {
            if (c == KEY_LF || c == KEY_RT)
                curmove(c, mode);
            if (c == KEY_ESC) {
                mode = V_READONLY;
                return 0;
            }

        }
    }

    for (int i = 0; i < len; i++) {
        cmd[i] = getcch(MAX_CHAR + i + 1);
    }

    return (char *) cmd;
}

static void
clearcli() {
    int i;
    for (i = MAX_COL * MAX_ROW; i < MAX_COL * MAX_ROW + 80; i++) {
        scrputc(i, (EMPTY_CHAR & 0xff) | BLACK);
    }
}

int
main(int argc, char *argv[]) {
    if (argc <= 1) {
        help();
        exit();
    }

    int current_line = 0;//note how many lines we scroll
    char * filename = argv[1];
    readfile(filename, &current_text);
    
    // TODO: Restore previous context
    int precursor = getcursor();

    clrscr();
    printwords(current_text.head, current_line);
    setcursor(0);
    setconsbuf(ENTER_VIM);

    int c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        printf(STDIN_FILENO, "Press %d\n", c);
        
        switch (c) {
            case KEY_UP:
            case KEY_DN:
            case KEY_LF:
            case KEY_RT:
                curmove(c, mode);
                break;
            case KEY_ESC:
                if (mode == V_INSERT) {
                    mode = V_READONLY;
                    curmove(KEY_LF, mode);
                }
                break;
            case ':': {
                if (mode == V_READONLY) {
                    mode = V_CMD;
                    scrputc(MAX_ROW * MAX_COL, ':');
                    char *cmd = readcmd();
                    if (cmd) {
                        // TODO pattern match
                        if (cmd[0] == 'q')
                            goto EXIT;
                        else if (cmd[0] == 'w' && cmd[1] == 'q')
                            goto SAVEANDEXIT;
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
            case '\b': {
                printf(0,"hello");
                break;
            }
            default: {
                if (mode == V_INSERT) {
                    int cp = getcursor();
                    int np;
                    pushword(cp, cp/MAX_COL);
                    if (c == '\n')
                        np = cp+MAX_COL - MAX_COL%MAX_COL;
                    else
                        np = cp + 1;
                    setcursor(np);
                    // scrputc(cp, c);
                    // putchar(cp+1,cc);
                    //change words linked list
                    current_text.head = addnode(current_text.head, current_line, cp, c);
                    printwords(current_text.head,0);
                    
                }
                break;
            }
        }
    }

    SAVEANDEXIT:
    writefile(filename, current_text.head);
    EXIT:

    setconsbuf(EXIT_VIM);
    clearcli();
    setcursor(precursor);
    exit();

}