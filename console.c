// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "console.h"
#include "color.h"
#include "vim.h"

static void consputc(int);

static int panicked = 0;
static int buffering = 1;  // If show buff in console

static struct {
    struct spinlock lock;
    int locking;
} cons;

static void
printint(int xx, int base, int sign) {
    static char digits[] = "0123456789abcdef";
    char buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';

    while (--i >= 0)
        consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...) {
    int i, c, locking;
    uint *argp;
    char *s;

    locking = cons.locking;
    if (locking)
        acquire(&cons.lock);

    if (fmt == 0)
        panic("null fmt");

    argp = (uint *) (void *) (&fmt + 1);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            consputc(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c) {
            case 'd':
                printint(*argp++, 10, 1);
                break;
            case 'x':
            case 'p':
                printint(*argp++, 16, 0);
                break;
            case 's':
                if ((s = (char *) *argp++) == 0)
                    s = "(null)";
                for (; *s; s++)
                    consputc(*s);
                break;
            case '%':
                consputc('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                consputc('%');
                consputc(c);
                break;
        }
    }

    if (locking)
        release(&cons.lock);
}

void
panic(char *s) {
    int i;
    uint pcs[10];

    cli();
    cons.locking = 0;
    // use lapiccpunum so that we can call panic from mycpu()
    cprintf("lapicid %d: panic: ", lapicid());
    cprintf(s);
    cprintf("\n");
    getcallerpcs(&s, pcs);
    for (i = 0; i < 10; i++)
        cprintf(" %p", pcs[i]);
    panicked = 1; // freeze other CPU
    for (;;);
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort *) P2V(0xb8000);  // CGA memory


static void
cgaputc(int c) {
    int pos;

    // Cursor position: col + 80*row.
    outb(CRTPORT, 14);
    pos = inb(CRTPORT + 1) << 8;
    outb(CRTPORT, 15);
    pos |= inb(CRTPORT + 1);

    if (c == '\n')
        pos += 80 - pos % 80;
    else if (c == BACKSPACE) {
        if (pos > 0) --pos;
    } else
        crt[pos++] = (c & 0xff) | 0x0700;  // black on white
//        crt[pos++] = (c & 0xff) | WHITE;  // white on black

    if (pos < 0 || pos > 25 * 80)
        panic("pos under/overflow");

    if ((pos / 80) >= 24) {  // Scroll up.
        memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
        pos -= 80;
        memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
    }

    outb(CRTPORT, 14);
    outb(CRTPORT + 1, pos >> 8);
    outb(CRTPORT, 15);
    outb(CRTPORT + 1, pos);
    crt[pos] = ' ' | 0x0700;
}


void
consputc(int c) {
    if (panicked) {
        cli();
        for (;;);
    }

    if (c == BACKSPACE) {
        if (!buffering) {
            cgaputc(c);
            return;
        }
        uartputc('\b');
        uartputc(' ');
        uartputc('\b');
    } else
        uartputc(c);
    cgaputc(c);
}

#define INPUT_BUF 128
struct {
    char buf[INPUT_BUF];
    uint r;  // Read index
    uint w;  // Write index
    uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

void
consoleintr(int (*getc)(void)) {
    int c, doprocdump = 0;

    acquire(&cons.lock);
    while ((c = getc()) >= 0) {
        switch (c) {
            case C('P'):  // Process listing.
                // procdump() locks cons.lock indirectly; invoke later
                doprocdump = 1;
                break;
            case C('U'):  // Kill line.
                if (buffering) {
                    while (input.e != input.w &&
                           input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
                        input.e--;
                        consputc(BACKSPACE);
                    }
                }
                break;
            case C('C'):  // Kill line.
                if (buffering) {
                    while (input.e != input.w &&
                           input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
                        input.e--;
                        consputc(BACKSPACE);
                    }
                }
                break;
            case C('H'):
            case '\x7f': { // Backspace
                if (buffering) {
                    if (input.e != input.w) {
                        input.e--;
                        consputc(BACKSPACE);
                    }
                } else {
                    // TODO: Bound check
                    int pos = getcursor();
                    int ch;
                    if (pos >= CMD_LINE) { // in command line
                        ch = (BACKSPACE & 0xff) | WHITE_ON_GREY;
                        if (pos > CMD_LINE) {
                            crt[pos - 1] = ch;
                            setcursor(pos - 1);
                        } else {
                            crt[pos] = ch;
                            setcursor(CMD_LINE);
                        }
                    } else {
                        ch = (BACKSPACE & 0xff) | WHITE;
                        if (pos > 0) {
                            crt[pos - 1] = ch;
                            setcursor(pos - 1);
                        } else {
                            crt[pos] = ch;
                            setcursor(0);
                        }
                    }


                }
            }
                break;
            default:
                if (c != 0 && input.e - input.r < INPUT_BUF) {
                    c = (c == '\r') ? '\n' : c;
                    input.buf[input.e++ % INPUT_BUF] = c;
                    if (buffering) {
                        consputc(c);
                    }
                    if (!buffering || c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF) {
                        input.w = input.e;
                        wakeup(&input.r);
                    }
                }
                break;
        }
    }
    release(&cons.lock);
    if (buffering && doprocdump) {
        procdump();  // now call procdump() wo. cons.lock held
    }
}

int
consoleread(struct inode *ip, char *dst, int n) {
    uint target;
    int c;

    iunlock(ip);
    target = n;
    acquire(&cons.lock);
    while (n > 0) {
        while (input.r == input.w) {
            if (myproc()->killed) {
                release(&cons.lock);
                ilock(ip);
                return -1;
            }
            sleep(&input.r, &cons.lock);
        }
        c = input.buf[input.r++ % INPUT_BUF];
        if (c == C('D')) {  // EOF
            if (n < target) {
                // Save ^D for next time, to make sure
                // caller gets a 0-byte result.
                input.r--;
            }
            break;
        }
        *dst++ = c;
        --n;
        if (c == '\n')
            break;
    }
    release(&cons.lock);
    ilock(ip);

    return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n) {
    int i;

    iunlock(ip);
    acquire(&cons.lock);
    for (i = 0; i < n; i++)
        consputc(buf[i] & 0xff);
    release(&cons.lock);
    ilock(ip);

    return n;
}

void
consoleinit(void) {
    initlock(&cons.lock, "console");

    devsw[CONSOLE].write = consolewrite;
    devsw[CONSOLE].read = consoleread;
    cons.locking = 1;

    ioapicenable(IRQ_KBD, 0);
}



// *******************************************************
// Self defined functions
// *******************************************************

static ushort precrt[MAX_ROW * MAX_COL];  // CGA memory

void savescr() {
    memmove(precrt, crt, (MAX_ROW * MAX_COL) * sizeof(ushort));
}

void restorescr() {
    memmove(crt, precrt, (MAX_ROW * MAX_COL) * sizeof(ushort));
}

int 
pushword(int pos, int line) {
    int i = (line+1) * MAX_COL-1;
    for ( ; i > pos; i--)
        crt[i]=crt[i-1];
    return crt[i];
}

void 
putchar(int pos, int c) {
    crt[pos] = (c & 0xff) | WHITE;
}

void
scrputc(int pos, int c) {
    setcursor(pos);

    int ch;
    if (pos >= CMD_LINE)
        ch = (c & 0xff) | WHITE_ON_GREY;
    else
        ch = (c & 0xff) | WHITE;

    if (c == '\n')
        pos += 80 - pos % 80;
    else if (c == BACKSPACE) {
        if (pos > 0) --pos;
    } else
        crt[pos++] = ch;
//        crt[pos++] = (c & 0xff) | WHITE;  // white on black
//
//    if (pos < 0 || pos > 25 * 80)
//        panic("pos under/overflow");
//
//    if ((pos / 80) >= 24) {  // Scroll up.
//        memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
//        pos -= 80;
//        memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
//    }

    outb(CRTPORT, 14);
    outb(CRTPORT + 1, pos >> 8);
    outb(CRTPORT, 15);
    outb(CRTPORT + 1, pos);
    if (pos >= CMD_LINE) {
        crt[pos] = ' ' | WHITE_ON_GREY;
    } else {
        crt[pos] = ' ' | L_GREY;
    }
}

int
getcursor() {
    int pos;

    outb(CRTPORT, 14);
    pos = inb(CRTPORT + 1) << 8;
    outb(CRTPORT, 15);
    pos |= inb(CRTPORT + 1);

    return pos;
}

int
getcch(int pos) {
    return crt[pos];
}

int
setcursor(int pos) {
    // TODO: boundary check


    outb(CRTPORT, 14);
    outb(CRTPORT + 1, pos >> 8);
    outb(CRTPORT, 15);
    outb(CRTPORT + 1, pos);
    int ch = crt[pos];
    if (ch == EMPTY_CHAR)
        crt[pos] = ch | WHITE;
    else
        crt[pos] = ch;
    return 0;
}

void curmove(int move, int mode) {
    // Textfield movement
    int pos = getcursor();

    switch (move) {
        case KEY_LF: {
            if (mode == V_CMD) {
                if (pos > CMD_LINE) {
                    setcursor(pos - 1);
                }
            } else {
                if (pos > 0) {
                    setcursor(pos - 1);
                }
            }
        }
            break;
        case KEY_RT: {
            if (mode == V_CMD) {
                if (pos < MAX_CHAR + CMD_BUF_SZ && (crt[pos + 1] & 0xff) != EMPTY_CHAR) {
                    setcursor(pos + 1);
                }
            } else {
                if (pos < MAX_CHAR && !((crt[pos + 1] & 0xff) == EMPTY_CHAR && (crt[pos] & 0xff) == EMPTY_CHAR))  {
                    setcursor(pos + 1);
                }
            }
        }
            break;
        case KEY_DN: {
            if (mode != V_CMD) {
                int crow = pos / 80;
                if (crow < MAX_ROW && (crt[pos + MAX_COL] & 0xff) != EMPTY_CHAR)
                    setcursor(pos + MAX_COL);
            }
        }
            break;
        case KEY_UP: {
            if (mode != V_CMD) {
                int crow = pos / 80;
                if (crow > 0)
                    setcursor(pos - MAX_COL && (crt[pos - MAX_COL] & 0xff) != EMPTY_CHAR);
            }
        }
            break;
        default:
            break;
    }
}

int
clrscr() {
    int pos = 0;
    int c = EMPTY_CHAR;

    for (int i = 0; i < MAX_COL * MAX_ROW; i++) {
        crt[i] = (c & 0xff) | L_GREY;
    }

    for (int i = 0; i < 80; i++) {
        crt[MAX_CHAR + i] = (EMPTY_CHAR & 0xff) | WHITE_ON_GREY;
    }
//    memset(crt, (ushort)((c & 0xff) | L_GREY), sizeof(crt[0]) * MAX_COL * MAX_ROW);

    outb(CRTPORT, 14);
    outb(CRTPORT + 1, pos >> 8);
    outb(CRTPORT, 15);
    outb(CRTPORT + 1, pos);
    crt[pos] = crt[pos] | WHITE;
    return 0;
}

/*
 * Control console
 */
void
setconsbuf(int isbuf) {
    buffering = isbuf;
}

// VIM Related
static text ctx;  // Text context

void initctx(char * filename) {
    ctx = {filename, 0, NULL, 0};
}

word *
addnode(word *words, int line, int pos, uchar c) {
    int current_pos = 0;
    int realpos = line * MAX_COL + pos;
    word *head = words;

    if (c == '\n') {
        int empty_length = MAX_COL - realpos % MAX_COL - 1;
        word *empty_head = (word *) malloc(sizeof(word));
        empty_head->blank = 1;
        empty_head->color = 0x21;
        empty_head->pre = NULL;
        empty_head->w = '\0';
        word *empty_node = empty_head;
        while (empty_length--) {
            empty_node->next = (word *) malloc(sizeof(word));
            empty_node->next->pre = empty_node;
            empty_node = empty_node->next;
            empty_node->w = '\0';
            empty_node->color = 0x21;
            empty_node->blank = 1;
        }

        if (realpos == 0) {
            empty_node->next = words;
            words->pre = empty_node;
            return empty_head;
        }
        for (current_pos = 0; current_pos < realpos;) {
            if (words->w == '\n')
                current_pos = (current_pos + 80) - (current_pos % MAX_COL);//reach the begining of next line
            else
                current_pos++;
            if (words->next != NULL)
                words = words->next;
        }
        if (words->next == NULL) {
            words->next = empty_head;
            empty_node->next = NULL;
            empty_head->pre = words;
        } else {
            empty_node->next = words;
            empty_head->pre = words->pre;
            words->pre->next = empty_head;
            words->pre = empty_node;
        }

    } else {
        word *new_word = (word *) malloc(sizeof(word));
        new_word->w = c;
        new_word->color = 0x21;
        new_word->blank = (c == '\n' ? 1 : 0);

        if (realpos == 0) {
            new_word->pre = NULL;
            new_word->next = words;
            words->pre = new_word;
            return new_word;
        }
        for (current_pos = 0; current_pos < realpos;) {
            if (words->w == '\n')
                current_pos = (current_pos + 80) - (current_pos % MAX_COL);//reach the begining of next line
            else
                current_pos++;
            if (words->next != NULL)
                words = words->next;
        }
        //check if it is the last char
        if (words->next == NULL) {
            words->next = new_word;
            new_word->next = NULL;
            new_word->pre = words;
        } else {
            new_word->next = words;
            new_word->pre = words->pre;
            new_word->pre->next = new_word;
            words->pre = new_word;
            //go on, check if blank=1 exists
            if (words->blank == 1)//delete a blank char
            {
                //zhijiegan!
                new_word->next = words->next;
                words->next->pre = new_word;
                free(words);
            } else {
                words = words->next;
                while (words != NULL) {
                    if (words->blank == 1) {
                        //gan!
                        words->pre->next = words->next;
                        words->next->pre = words->pre;
                        free(words);
                        break;
                    }
                    words = words->next;
                }
            }
        }
    }
    return head;
}

word *
create_text(int size, uchar *words) {
    int i, pos;
    word *head = NULL, *p;

    p = head = (word *) malloc(sizeof(word));

    for (i = 0, pos = 0; i < size;) {
        //chushihua

        if (words[i] == '\n') {
            int empty_length = MAX_COL - pos % MAX_COL;
            while (empty_length--) {
                p->next = (word *) malloc(sizeof(word));
                p->next->color = 0x21;
                p->next->w = '\0';
                p->next->blank = 1;
                p->next->pre = p;
                head->pre = p->next;//save current tail
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
            head->pre = p->next;//save current tail
            p = p->next;
            pos++;
        }
    }
    p = NULL;
    head->next->pre = head->pre;
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
printwords(struct word *words, int n_row)// print file begining from n_row row
{
    int pos = 0;
    int i, j;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < 80; j++) {
            if (words != NULL) {
                putchar(pos++, (int) words->w);
                words = words->next;
            } else
                putchar(pos++, (int) '\0');
        }
    }
    return 0;
}

// *******************************************************