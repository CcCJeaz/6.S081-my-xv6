#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  backtrace();
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void)
{
  struct proc *p = myproc();
  int ticks;
  uint64 handle;
  argint(0, &ticks);
  argaddr(1, &handle);
  if(ticks < 0) {
    return -1;
  }
  p->alarminfo.ticks = ticks;
  p->alarminfo.handle = (void (*)(void)) handle;
  p->alarminfo.time = 0;
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();
  if(p->alarminfo.ticks == 0)
    return -1;
  p->trapframe->a0  =  p->revert->a0;
  p->trapframe->a1  =  p->revert->a1;
  p->trapframe->a2  =  p->revert->a2;
  p->trapframe->a3  =  p->revert->a3;
  p->trapframe->a4  =  p->revert->a4;
  p->trapframe->a5  =  p->revert->a5;
  p->trapframe->a6  =  p->revert->a6;
  p->trapframe->a7  =  p->revert->a7;
  p->trapframe->epc =  p->revert->epc;
  p->trapframe->gp  =  p->revert->gp;
  p->trapframe->ra  =  p->revert->ra;
  p->trapframe->s0  =  p->revert->s0;
  p->trapframe->s1  =  p->revert->s1;
  p->trapframe->s2  =  p->revert->s2;
  p->trapframe->s3  =  p->revert->s3;
  p->trapframe->s4  =  p->revert->s4;
  p->trapframe->s5  =  p->revert->s5;
  p->trapframe->s6  =  p->revert->s6;
  p->trapframe->s7  =  p->revert->s7;
  p->trapframe->s8  =  p->revert->s8;
  p->trapframe->s9  =  p->revert->s9;
  p->trapframe->s10 =  p->revert->s10;
  p->trapframe->s11 =  p->revert->s11;
  p->trapframe->sp  =  p->revert->sp;
  p->trapframe->tp  =  p->revert->tp;
  p->trapframe->t0  =  p->revert->t0;
  p->trapframe->t1  =  p->revert->t1;
  p->trapframe->t2  =  p->revert->t2;
  p->trapframe->t3  =  p->revert->t3;
  p->trapframe->t4  =  p->revert->t4;
  p->trapframe->t5  =  p->revert->t5;
  p->trapframe->t6  =  p->revert->t6;

  p->done = 1;
  return 0;
}