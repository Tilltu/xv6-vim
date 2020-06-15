#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
  exit();
}