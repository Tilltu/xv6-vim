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
                if (pos < MAX_CHAR && (crt[pos + 1] & 0xff) != EMPTY_CHAR) {
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

void
setconsbuf(int isbuf) {
    buffering = isbuf;
}
// *******************************************************