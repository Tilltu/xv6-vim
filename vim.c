#include "vim.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "console.h"
#include "color.h"
#include "fcntl.h"

//text struct
typedef struct word {
    uchar w;
    uchar color;
    struct word *pre;
    struct word *next;
    int blank;        //check paragraph change
} word;

typedef struct text {
    char *path;
    int nbytes;             //number of words
    struct word *head;
    int ifexist;            //file exist?
} text;


static int mode = V_READONLY;

word *
create_text(int size, uchar *words) {
    int i, pos;
    word *head = NULL, *p;

    p = head = (word *) malloc(sizeof(word));

    for (i = 0, pos = 0; i < size;) {
        //chushihua    

        if (words[i] == '\n') {
            while (pos && pos % MAX_COL != 0) {
                p->next = (word *) malloc(sizeof(word));
                p->next->color = 0x21;
                p->next->w = '\0';
                p->next->blank = 1;
                p->next->pre = p;
                p = p->next;
                pos++;
            }
            i++;
        } else {
            p->next = (word *) malloc(sizeof(word));
            p->next->color = 0x21;
            p->next->w = words[i++];
            p->next->blank = 0;
            p->next->pre = p;
            p = p->next;
            pos++;
        }
    }
    p = NULL;
    head = head->next;
    return head;
}

int
readfile(char *path, struct text *t) {
    int fd;         //file decription
    struct stat st; //file info
    int f_size;     //size of file(bytes)
    uchar *words;   //store all words

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
    if (f_size > 0) {
        words = (uchar *) malloc(f_size);
        read(fd, words, f_size);    //get text to words(ok)
    } else                        //empty file
    {
        words = NULL;
    }

    read(fd, words, f_size);    //get text to words
    printf(1, "%d\n", f_size);
    close(fd);

    //build text struct
    t->ifexist = 1;
    t->nbytes = f_size;
    strcpy(t->path, path);
    t->head = create_text(f_size, words);
    return 0;
}

int
writefile(char *path, struct word *words) {
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
    while (words != NULL) {
        if (words->w != '\0') {
            write(fd, &words->w, 1);
            words = words->next;
        } else {
            while (words != NULL && words->blank) {
                words = words->next;
            }
            write(fd, "\n", 1);
        }
    }
    close(fd);
    return 0;
}


int
printword(struct word *words, int n_row)// print file begining from n_row row
{
    int pos = 0;
    int i, j;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < 80; j++) {
            // printc(pos++,'s');
            if (words != NULL) {
                scrputc(pos++, (int) words->w);
                words = words->next;
            } else {
                scrputc(pos++, (int) '\0');
            }

            // printf(1,"%c",words->w);

        }

    }
    return 0;
}


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

//    char * filename = argv[1];
    // TODO: Restore previous context
    int precursor = getcursor();

    clrscr();
    setcursor(0);
    setconsbuf(ENTER_VIM);

    int c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
//        printf(STDIN_FILENO, "Press %d\n", c);
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