#define MAX_ROW    24
#define MAX_COL    80
#define MAX_CHAR   MAX_ROW * MAX_COL
#define EMPTY_CHAR '\0'
#define MAX_FILE_SIZE 200000 // 1920 characters

// Mirror mode
#define M_OUT 0  // to kernel
#define M_IN  1  // to user


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

int getinput();

void deletech(int currentline, int linecount);
void mirrorctx(void* t, int mode);
void setline(int);
int  getline();
