#include "types.h"
word* addnode(struct word *,int, int,uchar);
word* create_text(int, uchar *);
int readfile(char *, struct text *);
int writefile(char *, struct word *);
int printwords(struct word *, int);
