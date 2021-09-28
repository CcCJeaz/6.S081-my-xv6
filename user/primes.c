#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void sieve(int* leftPipe) {
  close(leftPipe[1]);

  int buf[1];
  int flag = read(leftPipe[0], buf, 4);
  int prime = buf[0];
  
  if(flag == 0){
    close(leftPipe[0]);
    exit(0);
  }else {
    printf("prime %d\n", prime);
  }

  int rightPipe[2];
  pipe(rightPipe);

  if(fork() == 0) {
    close(leftPipe[0]);
    sieve(rightPipe);
  }else {
    close(rightPipe[0]);
    while(read(leftPipe[0], buf, 4) != 0) {
      if(buf[0] % prime != 0) {
        write(rightPipe[1], buf, 4);
      }
    }
    close(rightPipe[1]);
    close(leftPipe[0]);
    wait(0);
    exit(0);
  }
}

int main(int argc, char* argv[]) {
  int p[2];
  pipe(p);

  if(fork() == 0) {
    sieve(p);
  }else {
    close(p[0]);
    for(int i=2; i<36; i++) {
      write(p[1], &i, 4);
    }
    close(p[1]);
    wait(0);
  }
  exit(0);
}