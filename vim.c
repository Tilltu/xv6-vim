#include "vim.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "console.h"
#include "color.h"
#include "fcntl.h"


uchar text[MAX_FILE_SIZE];
uchar emptytext[MAX_FILE_SIZE];

static int vimline = 1;

void printtext(int);

int cursormove(int);

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

// File related
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
            memset(l->c, EMPTY_CHAR, sizeof(char) * 80);
            l->len = strlen(l->c);
            l->next = NULL;
            p = l;
            ctx->start = p;
            ctx->lines = 1;
        }
//        int test = 80 % 80;
//        printf(1, "P:%d\n", p);
//        printf(1, "Test:%d\n", test);
        //
        for (int i = 0; i < f_size; i++) {
//            if ((i + 1) % 80 == 0) {
//                p->len = strlen(p->c);
//                p->lineno = (i / 80) + 1;
//                // new line
//                line *l = (line *) malloc(sizeof(struct line));
//                l->lineno = p->lineno + 1;
//                memset(l->c, EMPTY_CHAR, sizeof(char) * 80);
//                l->len = strlen(l->c);
//                l->next = NULL;
//                if (buf[i] != '\n')
//                    l->para = p->para;
//                else
//                    l->para = p->para + 1;
//
//                p->next = l;
//                p = p->next;
//                ctx->lines++;
//
//            } else
            {
                if (buf[i] == '\n') {
                    p->len = strlen(p->c);
                    p->lineno = (i / 80) + 1;
                    // new line
                    line *nl = (line *) malloc(sizeof(struct line));
                    nl->lineno = p->lineno + 1;
                    memset(nl->c, EMPTY_CHAR, sizeof(char) * 80);

                    nl->len = strlen(nl->c);
                    if (nl->len < 80) {
                        nl->c[nl->len++] = '\n';
                    }
                    nl->next = NULL;
                    nl->para = p->para + 1;

                    p->next = nl;
                    p = p->next;
                    ctx->lines++;
                } else {
                    p->c[p->len++] = buf[i];
                    p->len = strlen(p->c);
                }
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
            l->len = 0;
            memset(l->c, EMPTY_CHAR, sizeof(char) * 80);

            l->next = NULL;
            p = l;
            ctx->start = p;
            ctx->lines = 1;
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

    while (p) {
//        printf(1, "Len of line %d:%d\n", i++, p->len);
        write(fd, &p->c, strlen(p->c));
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
        c == '$' || c == '%' ||
        c == '^' || c == '&' ||
        c == ':' || c == ' ')
        return 1;
    else
        return 0;
}

static int
inputfilter(int c) {
    // Viewable char: a-z A-Z 0-9 #./\+=-_!@#$%^&*
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '#' || c == '.' ||
        c == '/' || c == '\\' ||
        c == '+' || c == '-' ||
        c == '*' || c == '_' ||
        c == '!' || c == '@' ||
        c == '%' || c == '^' ||
        c == '&' || c == ':' ||
        c == '\n' || c == ' ' ||
        c == '$')
        return 1;
    else
        return 0;
}

static void
clearcli() {
    int i;
    for (i = MAX_COL * MAX_ROW; i < MAX_COL * MAX_ROW + 80; i++) {
        scrputc(i, (EMPTY_CHAR & 0xff) | BLACK);
    }
}

static void
showline() {
    int precur = getcursor();
    int i;
    for (i = MAX_COL * MAX_ROW; i < MAX_COL * MAX_ROW + 60; i++) {
        scrputc(i, (EMPTY_CHAR & 0xff) | BLACK);
    }
    int p = MAX_COL * MAX_ROW + 60;
    scrputc(p++, (('l') & 0xff) | WHITE);
    scrputc(p++, (('i') & 0xff) | WHITE);
    scrputc(p++, (('n') & 0xff) | WHITE);
    scrputc(p++, (('e') & 0xff) | WHITE);
    scrputc(p++, ((':') & 0xff) | WHITE);
    int d1;
    int d2;
    if (vimline >= 10) {
        d1 = vimline / 10;
        d2 = vimline % 10;
    } else {
        d1 = 0;
        d2 = vimline;
    }
    scrputc(p++, ((d1 + '0') & 0xff) | WHITE);
    scrputc(p++, ((d2 + '0') & 0xff) | WHITE);

    for (i = p; i < MAX_COL * MAX_ROW + 80; i++) {
        scrputc(i, (EMPTY_CHAR & 0xff) | BLACK);
    }

    setcursor(precur);
}

int getchar(int pos) {
    line *p = ctx->start;
    while (p->next && p->lineno < vimline)
        p = p->next;
    int col = (pos) % 80;
//    printf(1, "Col:%d", col);
    if (p == 0) {
        printf(2, "P is null");
        return 0;
    }
    return p->c[col];
}

// Cursor move wrapper
int
cursormove(int move) {
    int pos = getcursor();

    switch (move) {
        case KEY_LF: {
            if (mode == V_CMD) {
                if (pos > CMD_LINE) {
                    setcursor(pos - 1);
                }
            } else {
                if (pos > 0) {
                    if (getchar(pos - 1) != EMPTY_CHAR)
                        setcursor(pos - 1);
                }
            }
        }
            break;
        case KEY_RT: {
            if (mode == V_CMD) {
                if (pos < MAX_CHAR + CMD_BUF_SZ && (getcch(pos + 1)) != EMPTY_CHAR) {
                    setcursor(pos + 1);
                }
            } else {
                if (pos < MAX_CHAR && (getchar(pos + 1) != EMPTY_CHAR)) {
                    setcursor(pos + 1);
                }
            }
        }
            break;
        case KEY_DN: {
            if (mode != V_CMD) {
                int crow = pos / 80 + 1;
                if (crow <= MAX_ROW && crow < ctx->lines && (getchar(pos + MAX_COL) != EMPTY_CHAR)) {
                    if (pos + MAX_COL >= MAX_CHAR)
                        pos = MAX_CHAR - 1;
                    else
                        pos = pos + MAX_COL;
                    setcursor(pos);
                    upvimline();
                }
            }
        }
            break;
        case KEY_UP: {
            if (mode != V_CMD) {
                int crow = pos / 80 + 1;
                if (crow > 0 && (getchar(pos - MAX_COL) != EMPTY_CHAR)) {
                    if (pos - MAX_COL < 0)
                        pos = 0;
                    else
                        pos = pos - MAX_COL;
                    setcursor(pos);
                    downvimline();
                }
            }
        }
            break;
        default:
            break;
    }

    return 0;
}

// Remove character
void rmch() {
//    printf(1, "VIMLINE:%d", vimline);
    int pos = getcursor();
    int col = pos % MAX_COL;
//    int currentline = vimline;
    line *p = ctx->start;
    while (p->next && p->lineno < vimline)
        p = p->next;

    int linecount = 0;
    if (p->next != NULL) {
        line *parap = p->next;
        while (parap && parap->para == p->para) {
            parap = parap->next;
            linecount++;
        }
    }
//    int i;
    int len = strlen(p->c);
//    printf(2, "Len:%d", len);
//    printf(2, "Col:%d", col);
//    printf(1, "Before:%d\n", len);
    if (len < MAX_COL) {
//        for (i = col; i < len - 1; i++) {
//            p->c[i] = p->c[i + 1];
//        }
//        p->c[i] = EMPTY_CHAR;
        memmove(p->c + col, p->c + col + 1, sizeof(char) * 80 - col);
    } else { // TODO: hava same paragraph line

    }
//    printf(1, "After:%d\n", strlen(p->c));


    // Screen delete
//    deletech(vimline, linecount);
    printtext(1);
    setcursor(pos);
}

char *
readcmd() {
    int precur = getcursor();
    int pos;
    int len;

    clearcli();
    scrputc(CMD_LINE, ':');  // set cursor to CMD_LIME

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
                cursormove(c);
            if (c == KEY_ESC) {
                mode = V_READONLY;
                clearcli();
                setcursor(precur);
                return 0;
            }

        }
    }

    for (int i = 0; i < len; i++) {
        cmd[i] = getcch(MAX_CHAR + i + 1); // 1 for ':'
    }


    return (char *) cmd;
}


void
printtext(int line) {
//    int precur = getcursor();
    int pos = 0;
    int i;

    struct line *p = ctx->start;
    while (p->next && p->lineno < line) {
        p = p->next;
    }
    while (p) {
        for (i = 0; i < strlen(p->c); i++) {
            if (p->c[i] == '\n') {
                // TODO: Bound check
                pos = pos + 80 - ((pos + 1) % 80);
                scrputc(pos, '\n');
            }
//            if (p->c[i] != EMPTY_CHAR)
            scrputc(pos++, (p->c[i]));
        }
        p = p->next;
    }

    setcursor(0);
}

int
main(int argc, char *argv[]) {
    if (argc <= 1) {
        help();
        exit();
    }

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
    setcursor(0);
    setline(1);
    printtext(1);
    setconsbuf(ENTER_VIM);

    int c;
    while (read(STDIN_FILENO, &c, sizeof(uchar)) == 1) {
        showline();
        switch (c) {
            case KEY_UP:
            case KEY_DN:
            case KEY_LF:
            case KEY_RT:
                cursormove(c);
                break;
            case 'k': // up
            {
                if (mode == V_INSERT) {
                    cursormove(KEY_UP);
                } else
                    goto DEFAULT;
                break;
            }
            case 'j': // down
            {
                if (mode == V_READONLY) {
                    cursormove(KEY_DN);
                } else
                    goto DEFAULT;
                break;
            }
            case 'h': // left
            {
                if (mode == V_READONLY) {
                    cursormove(KEY_LF);
                } else
                    goto DEFAULT;
                break;
            }
            case 'l': // right
            {
                if (mode == V_READONLY) {
                    cursormove(KEY_RT);
                } else
                    goto DEFAULT;
                break;
            }
            case KEY_ESC:
                if (mode == V_INSERT) {
                    mode = V_READONLY;
                    cursormove(KEY_LF);
                } else if (mode == V_CMD) {
                    mode = V_READONLY;
                }
                break;
            case ':': {
                if (mode == V_READONLY) {
                    mode = V_CMD;
                    char *cmd = readcmd();
//                    printf(2, "CMD:%s", cmd);
                    if (cmd) {
                        // TODO pattern match
                        if (cmd[0] == 'q')
                            goto EXIT;
                        else if (cmd[0] == 'w' && cmd[1] == 'q') {
                            goto SAVEANDEXIT;
                        } else {
                            setcursor(CMD_LINE);
//                            printf(1, "Wrong cmd");// TODO: prompt failure
//                            break;
                        }
                    }
                } else if (mode == V_INSERT) {
                    goto DEFAULT;
                }
            }
                break;
            case 'i': {
                // insert mode
                if (mode == V_READONLY) {
                    mode = V_INSERT;
                } else if (mode == V_INSERT) {
                    goto DEFAULT;
                }
                break;
            }
            case 'x': {
                // delete character
                if (mode == V_READONLY) {
                    rmch();
                } else if (mode == V_INSERT) {
                    goto DEFAULT;
                }
            }
                break;
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
                    p->c[strlen(p->c)] = '\n';
                }

                l->lineno = lineno + 1;
                l->para = para + 1;
                memset(l->c, EMPTY_CHAR, sizeof(char) * 80);

                l->len = strlen(l->c);
                l->next = NULL;
                p->next = l;
                ctx->lines++;

                scrputc(getcursor(), '\n');

            }
                break;

            DEFAULT:
            default: {
                if (mode == V_INSERT) {
                    if (!inputfilter(c))
                        continue;
                    int cp = getcursor();
//                    int np;
//                    pushword(cp, cp / MAX_COL);
//                    if (c == '\n')
//                        np = cp + MAX_COL - cp % MAX_COL;
//                    else
//                        np = cp + 1;
                    setcursor(cp);
                    scrputc(cp, c);

                    line *p = ctx->start;
                    if (p == NULL) {
                        line *l = (line *) malloc(sizeof(struct line));
                        l->lineno = 1;
                        l->para = 1;
                        l->len = 0;
                        memset(l->c, EMPTY_CHAR, sizeof(char) * 80);

                        l->next = NULL;
                        p = l;
                        ctx->start = p;
                        ctx->lines = 1;
                    } else {
//                        printf(1, "LINE:%d", vimline);
                        while (p->next && p->lineno < vimline) {
                            p = p->next;
                        }

                        int col = (cp) % 80;
                        if (col == 0 && cp != 0 && p->len == 80) { // new line
                            upvimline();
                            setline(vimline);
                            line *nl = (line *) malloc(sizeof(struct line));
                            line *np = ctx->start;
                            while (np->next)
                                np = np->next;
                            int npara = np->para;
                            int nlineno = np->lineno;

                            nl->lineno = nlineno + 1;
                            nl->para = npara;
                            memset(nl->c, EMPTY_CHAR, sizeof(char) * 80);

                            nl->c[col] = c;
                            nl->len = strlen(nl->c);
                            nl->next = NULL;
                            np->next = nl;

                            ctx->lines++;
                        } else {
//                            printf(1, "P-lineno:%d", p->lineno);
                            p->c[col] = c;
                            p->len = strlen(p->c);
                        }
                    }
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

    exit();

}