#define MAX_ROW    24
#define MAX_COL    80
#define MAX_CHAR   MAX_ROW * MAX_COL
#define EMPTY_CHAR '\0'

int  clrscr();
int  getcursor();
int  setcursor(int);
void setconsbuf(int);
void scrputc(int, int);
int  getcch(int);
int pushword(int,int);
void putchar(int,int);

void curmove(int, int);

void savescr();
void restorescr();