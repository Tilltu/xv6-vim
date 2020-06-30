#include "vim.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "console.h"
#include "color.h"
#include "fcntl.h"

uchar text[MAX_FILE_SIZE];
uchar emptytext[MAX_FILE_SIZE];

static int vimline = 0;

void
upvimline() {
    vimline++;
}

void
downvimline() {
    vimline--;
}

void
setvimline(int line) {
    vimline = line;
}

struct ctext *ctx;

void initctx() {
    ctx = (struct ctext *) malloc(sizeof(struct ctext));
}

int
readfile(char *path) {
    int fd;         //file decription
    struct stat st; //file info
    int f_size;     //size of file(bytes)
//    uchar *words;   //store all words

    // open a file and get stat
    if (path != NULL && (fd = open(path, O_RDONLY | O_CREATE)) >= 0) {
        if (fstat(fd, &st) < 0) {
            printf(2, "Can not get file stat %s\n", path);
            close(fd);
            return -1;
        }
        if (st.type != T_FILE) {//not a file
            printf(2, "What you open is not a FILE!");
            close(fd);
            return -1;
        }
    }
        //read error
    else {
        printf(2, "Can not open %s\n", path);
        return -1;
    }
    f_size = st.size;           //get file size(bytes)
    if (f_size > MAX_FILE_SIZE) {
        printf(2, "File too big, over %d", MAX_FILE_SIZE);
        close(fd);
        return -1;
    }
    if (f_size > 0) {
//        read(fd, text, f_size);    //get text to words(ok)
//        printf(1, "HERE:");
        char buf[f_size];
        read(fd, buf, f_size);
//        printf(1, "%c", buf[0]);

        line *p = ctx->start;
        if (p == NULL) {
            line *l = (line *) malloc(sizeof(struct line));
            l->lineno = 1;
            l->para = 1;
            l->len = strlen(l->c);
            l->next = NULL;
            p = l;
            ctx->start = p;
        }
//        int test = 80 % 80;
//        printf(1, "P:%d\n", p);
//        printf(1, "Test:%d\n", test);
        //
        for (int i = 0; i < f_size; i++) {
            if ((i + 1) % 80 == 0) {
                p->len = strlen(p->c);
                p->lineno = i / 80;
                // new line
                line *l = (line *) malloc(sizeof(struct line));
                l->lineno = p->lineno + 1;
                l->len = strlen(l->c);
                l->next = NULL;
                if (buf[i] != '\n')
                    l->para = p->para;
                else
                    l->para = p->para + 1;

                p->next = l;
                p = p->next;
            } else {
                p->c[i % 80] = buf[i];
                p->len++;
            }
        }
    } else                        //empty file
    {
//        memset(text, EMPTY_CHAR, MAX_FILE_SIZE);
        line *p = ctx->start;
        if (p == NULL) {
            line *l = (line *) malloc(sizeof(struct line));
            l->lineno = 1;
            l->para = 1;
            l->len = strlen(l->c);
            l->next = NULL;
            p = l;
            ctx->start = p;
        }
    }

//    read(fd, words, f_size);    //get text to words
    printf(1, "%d\n", f_size);
    close(fd);

    return 0;
}

int
writefile(char *path) {
    int fd;         //file decription
    struct stat st; //file info
    // open a file and get stat
    if (path != NULL && (fd = open(path, O_WRONLY | O_CREATE)) >= 0) {
        if (fstat(fd, &st) < 0) {
            printf(2, "Can not get file stat \n");
            close(fd);
            return -1;
        }
        if (st.type != T_FILE) {//not a file
            printf(2, "What you open is not a FILE!");
            close(fd);
            return -1;
        }
    }
        //write error
    else {
        printf(2, "Can not write\n");
        return -1;
    }

    //start writing
    line *p = ctx->start;
//    int i;
    while (p) {
        write(fd, &p->c, p->len);
        p = p->next;
    }
//    for (int i = 0; i < MAX_FILE_SIZE; i++) {
//        if (text[i] == '\0') {
//            break;
//        }
//        write(fd, &text[i], 1);
//    }
    close(fd);
    return 0;
}


static int mode = V_READONLY;

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
        c == '&' || c == ':')
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
    while (read(STDIN_FILENO, &c, sizeof(uchar)) == 1 && c != '\n') {
        pos = getcursor();
        len = (pos - MAX_CHAR) % MAX_COL;
        if (len > CMD_BUF_SZ) {
            printf(STDOUT_FILENO, "len=%d", len);
            return NULL; // TODO: Better way to handle
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

void
printtext() {
    int precur = getcursor();
    int pos = precur;
    int i;
//    for (i = 0; i < MAX_CHAR; i++) {
//        scrputc(i, text[i]);
//    }
    line *p = ctx->start;
//    printf(1, "%d\n", p);
    while (p) {
//        printf(1, "p->len:%d\n", p->len);
        for (i = 0; i < p->len; i++) {
//            printf(1, "%c\n", (p->c[i]));
            if (p->c[i] == '\n') {
                // TODO: Bound check
                pos = pos + 80 - ((pos + 1) % 80);
                scrputc(pos, '\n');
            }
            scrputc(pos++, (p->c[i]));
        }
//        scrputc(pos++, '\n');
        p = p->next;
    }

    setcursor(precur);
}

int
main(int argc, char *argv[]) {
    if (argc <= 1) {
        help();
        exit();
    }

//    int current_line = 0;//note how many lines we scroll
    char *filename = argv[1];
    initctx();
    if (readfile(filename) != 0) {
        exit();
    }
    if (ctx == NULL) {
        printf(1, "ctx is null");
        exit();
    }
    if (ctx->start == NULL) {
        printf(1, "ctx is null");
        exit();
    }

    // Restore previous context
    savescr();
    int precursor = getcursor();

    clrscr();
//    printwords(current_text.head, current_line);
    setcursor(0);
    printtext();
    setconsbuf(ENTER_VIM);
    mirrorctx(ctx, M_IN);

    int c;
//    while (read(STDIN_FILENO, &c, 1) == 1) {
    while (read(STDIN_FILENO, &c, sizeof(uchar)) == 1) {
//        mirrorctx(text, M_IN);
//        printtext();
//        deletech(ctx, (getcursor() + 1) % 80);
        deletech(ctx, (getcursor() + 1) % 80);
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
                } else if (mode == V_CMD) {
                    mode = V_READONLY;
                }
                break;
            case ':': {
                if (mode == V_READONLY) {
                    clearcli();
                    mode = V_CMD;
                    scrputc(MAX_ROW * MAX_COL, ':');
                    char *cmd = readcmd();
                    if (cmd) {
                        // TODO pattern match
                        if (cmd[0] == 'q')
                            goto EXIT;
                        else if (cmd[0] == 'w' && cmd[1] == 'q') {

                            goto SAVEANDEXIT;
                        }
                    }
                } else if (mode == V_INSERT) {
                    goto DEFAULT;
                }
                break;
            }
            case 'i': {
                // insert mode
                if (mode == V_READONLY) {
                    mode = V_INSERT;
                } else if (mode == V_INSERT) {
                    goto DEFAULT;
//                    current_text.head = addnode(current_text.head, current_line, cp, c);
                }
                break;
            }
//            case 'x': TODO:
//                break;
            case '\n': {
                upvimline();
                setline(vimline);
                line *l = (line *) malloc(sizeof(struct line));
                line *p = ctx->start;
                while (p->next)
                    p = p->next;
                int para = p->para;
                int lineno = p->lineno;
                if (p->len < 80) {
                    p->c[p->len++] = '\n';
                }

                l->lineno = lineno + 1;
                l->para = para + 1;
                l->len = strlen(l->c);
                l->next = NULL;
                p->next = l;

                scrputc(getcursor(), '\n');
            }
                break;
            DEFAULT:
            default: {
//                printf(1, "%d\n", c);
                if (mode == V_INSERT) {
                    int cp = getcursor();
                    int np;
//                    pushword(cp, cp / MAX_COL);
                    if (c == '\n')
                        np = cp + MAX_COL - cp % MAX_COL;
                    else
                        np = cp + 1;
                    setcursor(np);
                    scrputc(cp, c);

                    line *p = ctx->start;
                    if (p == NULL) {
                        line *l = (line *) malloc(sizeof(struct line));
                        l->lineno = 1;
                        l->para = 1;
                        l->len = strlen(l->c);
                        l->next = NULL;
                        p = l;
                        ctx->start = p;
                    } else {
                        while (p && p->lineno < vimline) {
                            p = p->next;
                        }
                        int col = (cp + 1) % 80;
//                        int len = p->len;
                        p->c[col] = c;
                        p->len++;
                    }


//                    printf(1, "%d\n", text[cp]);
                    // putchar(cp+1,cc);
                    //change words linked list
//                    current_text.head = addnode(current_text.head, current_line, cp, c);
//                    printwords(current_text.head, 0);

                }
                break;
            }
        }
    }


    SAVEANDEXIT:
    writefile(filename);

    EXIT:
    setconsbuf(EXIT_VIM);
    clearcli();
    restorescr();
    setcursor(precursor);
//    mirrorctx(NULL, M_OUT);
    exit();

}