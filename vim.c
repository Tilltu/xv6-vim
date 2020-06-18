#include "vim.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "console.h"

void help() {
    printf(STDOUT_FILENO, "Usage: vim [FILENAME]\n");
}

int
main(int argc, char *argv[]) {
    if (argc <= 1) {
        help();
        exit();
    }


    clrscr();
    setcursor(0);
    entercon();

    uchar c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        printf(0, "you hit %d", c);
        switch (c) {
            case KEY_RT:
                printf(0, "you hit right button");
                setcursor(1);
                break;

            default:
                break;
        }
    }

    exit();
}