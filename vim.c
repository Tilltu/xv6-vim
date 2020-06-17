#include "vim.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "console.h"

// Last line for command input
static char textfild[MAX_ROW + 1][MAX_COL];

struct cursor {
  char avatar;
  int prerow;
  int precol;
  int nowrow;
  int nowcol;
} cur;

// static cursor cur;

void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

void help() {
  printf(STDOUT_FILENO, "Usage: vim [FILENAME]\n");
}

void
initscr() { 
  cur.prerow = CURSOR_INIT_ROW;
  cur.precol = CURSOR_INIT_COL;
  cur.nowrow = CURSOR_INIT_ROW;
  cur.nowcol = CURSOR_INIT_COL;
  cur.avatar = CURSOR_AVATAR;

  printf(STDOUT_FILENO, "MAX_ROW:%d\n", MAX_ROW);
  printf(STDOUT_FILENO, "MAX_COL:%d\n", MAX_COL);

  for(int i = 0;i < MAX_ROW;i++) {
    for(int j = 0;j < MAX_COL;j++) {
      textfild[i][j] = ' ';
    }
  } 
  textfild[cur.nowrow][cur.nowcol] = cur.avatar;
}

void 
flushscr() {
  textfild[cur.prerow][cur.precol] = ' ';
  textfild[cur.nowrow][cur.nowcol] = cur.avatar;
  // for(int i = 0;i < MAX_ROW;i++) 
  //   for(int j = 0;j < MAX_COL;j++) {
  //     // uartputc('\b'); uartputc(' '); uartputc('\b');
  //     putc(STDOUT_FILENO, '\b'); putc(STDOUT_FILENO, ' '); putc(STDOUT_FILENO, '\b');
  //   }
      

  for(int i = 0;i < MAX_ROW;i++) {
    for(int j = 0;j < MAX_COL;j++) {      
      putc(STDOUT_FILENO, textfild[i][j]);
      if (j == MAX_COL - 1)
        putc(STDOUT_FILENO, '\n');
    }
  }
}


void movecur(char dir) {
  int validmove = 0;
  switch (dir)
  {
  case UP:
    if (cur.nowrow + 1 < MAX_ROW) {
      cur.nowrow += 1;
      validmove = 1;
    }
    break;
  case DOWN:
    if (cur.nowrow - 1 >= 0) {
      cur.nowrow -= 1;
      validmove = 1;
    }
    break;
  case LEFT:
    if (cur.nowcol - 1 >= 0) {
      cur.nowcol -= 1;
      validmove = 1;
    }
    break;
  case RIGHT:
    if (cur.nowcol + 1 < MAX_COL) {
      cur.nowrow -= 1;
      validmove = 1;
    }
    break;
  
  default:
    break;
  }

  if (validmove) {
    flushscr();
  }
}

void 
save(const char *filename) {
  int fd;
  
  // if(open(filename, O_RDWR) < 0){
  //   mknod(filename, 1, 1);
  //   open(filename, O_RDWR);
  // }

  if((fd = open(filename, 0)) < 0){
      printf(1, " cannot open %s\n", filename);
      if (mknod(filename, 1, 1) < 0) {
        printf(1, " cannot maje node %s\n", filename);
        exit();
      }
      
      if((fd = open(filename, 0)) < 0){
        printf(1, " cannot open %s\n", filename);
        exit();
      }   
  }

  int ret = write(fd, textfild, sizeof(textfild));
  if (ret != 0) {
    printf(1, "write error");
    exit();
  }
}

int 
main(int argc, char* argv[]) {
  if (argc <= 1) {
    help();
    exit();
  }

  clrscr();

  // initscr();
  // flushscr();
  
  // // printf("%s", argv[1]);

  // char c;
  // while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
  //   switch (c)
  //   {
  //   case LEFT:
  //   case RIGHT:
  //   case UP:
  //   case DOWN:
  //     movecur(c);
  //     break;
    
  //   case 'w':
  //     save(argv[1]);
  //     break;
  //   default:
  //     break;
  //   }
  // }
  exit();
}