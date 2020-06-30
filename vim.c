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

void printtext();

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
            l->len = 0;
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

// Cursor move wrapper
int
cursormove(int move) {
    return 0;
}

// Remove character
void rmch() {
    int pos = getcursor();
    int col = pos % MAX_COL;
    int currentline = vimline;
    line *p = ctx->start;
    while (p && p->lineno < currentline)
        p = p->next;

    int linecount = 0;
    if (p->next != NULL) {
        line *parap = p->next;
        while (parap && parap->para == p->para) {
            parap = parap->next;
            linecount++;
        }
    }
    int i;
    int len = strlen(p->c);
    if (len < MAX_COL) {
        for (i = col; i < len - 1; i++) {
            p->c[i] = p->c[i + 1];
        }
        p->c[i] = EMPTY_CHAR;
    } else { // hava same paragraph line

    }


    // Screen delete
//    deletech(vimline, linecount);
    printtext();
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
                curmove(c, mode);
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
printtext() {
//    int precur = getcursor();
    int pos = 0;
    int i;

    line *p = ctx->start;
    while (p) {
        for (i = 0; i < p->len; i++) {
            if (p->c[i] == '\n') {
                // TODO: Bound check
                pos = pos + 80 - ((pos + 1) % 80);
                scrputc(pos, '\n');
            }
            if (p->c[i] != EMPTY_CHAR)
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
    printtext();
    setconsbuf(ENTER_VIM);

    int c;
    while (read(STDIN_FILENO, &c, sizeof(uchar)) == 1) {

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
                        l->next = NULL;
                        p = l;
                        ctx->start = p;
                    } else {
                        while (p && p->lineno < vimline) {
                            p = p->next;
                        }
                        int col = (cp) % 80;
                        if (col == 0 && cp != 0) { // new line
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
                            nl->c[col] = c;
                            nl->len = strlen(nl->c);
                            nl->next = NULL;
                            np->next = nl;
                        } else {
                            p->c[col] = c;
                            p->len++;
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