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
int  pushword(int,int);
void putchar(int,int);

void curmove(int, int);

void savescr();
void restorescr();



//******************
// VIM Related
void initctx(char *filename);

word *
addnode(word *words, int line, int pos, uchar c);

word *
create_text(int size, uchar *words);

int
readfile(char *path, struct text *t);

int
writefile(char *path, struct word *words);

int
printwords(struct word *words, int n_row);