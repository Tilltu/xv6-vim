#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "console.h"
#include "vim.h"

int
sys_fork(void) {
    return fork();
}

int
sys_exit(void) {
    exit();
    return 0;  // not reached
}

int
sys_wait(void) {
    return wait();
}

int
sys_kill(void) {
    int pid;

    if (argint(0, &pid) < 0)
        return -1;
    return kill(pid);
}

int
sys_getpid(void) {
    return myproc()->pid;
}

int
sys_sbrk(void) {
    int addr;
    int n;

    if (argint(0, &n) < 0)
        return -1;
    addr = myproc()->sz;
    if (growproc(n) < 0)
        return -1;
    return addr;
}

int
sys_sleep(void) {
    int n;
    uint ticks0;

    if (argint(0, &n) < 0)
        return -1;
    acquire(&tickslock);
    ticks0 = ticks;
    while (ticks - ticks0 < n) {
        if (myproc()->killed) {
            release(&tickslock);
            return -1;
        }
        sleep(&ticks, &tickslock);
    }
    release(&tickslock);
    return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void) {
    uint xticks;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);
    return xticks;
}

int
sys_clrscr(void) {
    return clrscr();
}

int
sys_getcursor(void) {
    return getcursor();
}

int
sys_setcursor(int pos) {
    argint(0, &pos);
    return setcursor(pos);
}

int
sys_setconsbuf(int isbuf) {
    argint(0, &isbuf);
    setconsbuf(isbuf);
    return 0;
}

int
sys_scrputc(int pos, int c) {
    argint(0, &pos);
    argint(1, &c);
    scrputc(pos, c);
    return 0;
}

int
sys_curmove(int move, int mode) {
    argint(0, &move);
    argint(1, &mode);
    curmove(move, mode);
    return 0;
}

int
sys_getcch(int pos) {
    argint(0, &pos);
    return getcch(pos);
}

int
sys_pushword(int pos, int line) {
    argint(0, &pos);
    argint(1, &line);
    return pushword(pos, line);
}

int
sys_putchar(int pos, int c) {
    argint(0, &pos);
    argint(1, &c);
    putchar(pos, c);
    return 0;
}

int
sys_savescr() {
    savescr();
    return 0;
}

int
sys_restorescr() {
    restorescr();
    return 0;
}

int
sys_mirrorctx(struct ctext *t, int mode) {
    argptr(0, (void *) &t, sizeof(*t));
    argint(1, &mode);
    mirrorctx(t, mode);
    return 0;
}

int
sys_getline() {
    return getline();
}

int
sys_setline(int l) {
    argint(0, &l);
    setline(l);
    return 0;
}

int
sys_deletech(int currentline, int linecount) {
    argint(0, &currentline);
    argint(1, &linecount);
    deletech(currentline, linecount);
    return 0;
}

int
sys_getinput() {
    return getinput();
}