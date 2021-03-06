#include "types.h"

#define KBSTATP         0x64    // kbd controller status port(I)
#define KBS_DIB         0x01    // kbd data in buffer
#define KBDATAP         0x60    // kbd data port(I)

#define NO              0

#define SHIFT           (1<<0)
#define CTL             (1<<1)
#define ALT             (1<<2)

#define CAPSLOCK        (1<<3)
#define NUMLOCK         (1<<4)
#define SCROLLLOCK      (1<<5)

#define E0ESC           (1<<6)
#define KEY_ESC         0x1B

// Special keycodes
#define KEY_HOME        0xE0
#define KEY_END         0xE1
#define KEY_UP          0xE2
#define KEY_DN          0xE3
#define KEY_LF          0xE4
#define KEY_RT          0xE5
#define KEY_PGUP        0xE6
#define KEY_PGDN        0xE7
#define KEY_INS         0xE8
#define KEY_DEL         0xE9
#define C(x)  ((x)-'@')  // Control-x

// Control if console buffer
#define ENTER_VIM       0
#define EXIT_VIM        1

#define CMD_BUF_SZ      40

// Mode
#define V_READONLY      1
#define V_INSERT        2
#define V_CMD           3

// Command line start
#define CMD_LINE MAX_ROW * MAX_COL

#define NULL 0


//#define MAX_FILE_SIZE   2048 // 2048 characters
typedef struct line {
    char c[80];
//    char color;
    int len;
    int lineno;
    int para; // paragraph number
    struct line *next;
} line;

typedef struct ctext {
    line *start;
    int lines; // number of lines
} ctext;