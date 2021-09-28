#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
  int p1[2];
  int p2[2];
  pipe(p1);
  pipe(p2);

  if(fork() == 0) {
    int PID = getpid();
    char rbuf[1];
    char wbuf[1];
    
    close(p1[1]);
    read(p1[0], rbuf, 1);
    close(p1[0]);

    wbuf[0] = 2;
    write(p2[1], wbuf, 1);
    close(p2[1]);
    close(p2[0]);

    printf("%d: received ping\n", PID);

  }else {
    int PID = getpid();
    char rbuf[1];
    char wbuf[1];
    
    close(p2[1]);

    wbuf[0] = 1;
    write(p1[1], wbuf, 1);
    close(p1[1]);
    close(p1[0]);

    wait(0);

    read(p2[0], rbuf, 1);
    close(p2[0]);

    printf("%d: received pong\n", PID);
  }
  exit(0);
}