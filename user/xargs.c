#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
  char buf[100];
  char *bp = buf;
  char *p = buf;
  char *arr[MAXARG];
  for(int i=1; i<argc; i++) {
    arr[i-1] = argv[i];
  }
  while(read(0, p, 1) != 0) {
    if(*p == '\n' || *p == 0) {
      *p = 0;
      if(fork() == 0) {
        arr[argc-1] = bp;
        exec(arr[0], arr);
      }else {
        bp = p+1;
        wait(0);
      }
    }
    p++;
  }
  exit(0);
}