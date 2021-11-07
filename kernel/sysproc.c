#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

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

pte_t *walk(pagetable_t pagetable, uint64 va, int alloc);
uint64
sys_mmap(void)
{
  struct proc *p = myproc();

  int len, prot, flag, fd;
  int i;

  uint64 addr = -1;
  //提取参数
  if(argint(1, &len) < 0   || len < 1 ||
     argint(2, &prot) < 0  || 
     argint(3, &flag) < 0  ||
     argint(4, &fd) < 0    || fd < 0 ) {
    return -1;
  }

  //fd是否存在?
  if(p->ofile[fd] == 0) {
    return -1;
  }


  //检查文件权限
  if(flag & MAP_SHARED) {
    if(((!p->ofile[fd]->writable) && prot & PROT_WRITE) ||
      ((!p->ofile[fd]->readable) && prot & PROT_READ) ){
      return -1;
    }
  }

  //定住file
  filedup(p->ofile[fd]);
  // idup(p->ofile[fd]->ip);

  
  //将数据写入
  for(i=0; i<16; i++) {
    //寻找空的VMA
    if(p->vma_list[i].len == 0) {
      addr = PGROUNDDOWN(p->top - len);
      
      if(addr < p->sz){
        printf("warning: mmap no size!\n");
        return -1;
      }
      p->vma_list[i].fd = fd;
      p->vma_list[i].flag = flag;
      p->vma_list[i].prot = prot;
      p->vma_list[i].len = len;
      p->vma_list[i].ip = p->ofile[fd]->ip;
      p->vma_list[i].addr = addr;
      p->vma_list[i].file_start = addr;
      p->vma_list[i].f = p->ofile[fd];
      p->top -= PGROUNDUP(len);
      return addr;
    }
  }
  return -1;
}

int my_munmap(uint64 addr, int len) {
  struct proc *p = myproc();
  //查找对应的vma
  int idx;
  for(idx = 0; idx < 16; idx++) {
    if(p->vma_list[idx].addr <= addr &&
        p->vma_list[idx].addr + p->vma_list[idx].len > addr && p->vma_list[idx].len != 0)
      break;
  }
  if(idx == 16)
    return -1;
  
  //计算实际长度, 最大不能超过map的结尾
  int n = len;
  if(addr + len > p->vma_list[idx].addr + p->vma_list[idx].len) 
    n = p->vma_list[idx].addr + p->vma_list[idx].len - addr;
  
  //写回
  if(p->vma_list[idx].flag & MAP_SHARED) {
    
    for (uint64 i = PGROUNDDOWN(addr); i < PGROUNDUP(addr+n); i+=PGSIZE) {
      if(walkaddr(p->pagetable, i)) {
        begin_op();
        ilock(p->vma_list[idx].ip);
        writei(p->vma_list[idx].ip, 1, i, i - p->vma_list[idx].file_start, PGSIZE/2);
        iunlock(p->vma_list[idx].ip);
        end_op();

        begin_op();
        ilock(p->vma_list[idx].ip);
        writei(p->vma_list[idx].ip, 1, i+PGSIZE/2, i+PGSIZE/2 - p->vma_list[idx].file_start, PGSIZE/2);
        iunlock(p->vma_list[idx].ip);
        end_op();
      }
        
    }
  }

  //计算释放内存范围
  uint64 start = PGROUNDUP(addr),
         end = PGROUNDDOWN(addr + n);
  if(start != p->vma_list[idx].addr)
    end = p->vma_list[idx].addr + p->vma_list[idx].len;
  
  for(uint64 i=start; i<end; i+=PGSIZE)
    if(walkaddr(p->pagetable, i))
      uvmunmap(p->pagetable, i, 1, 1);
  
  //更新vma_list信息
  p->vma_list[idx].len -= n;
  if(start == p->vma_list[idx].addr) {
    p->vma_list[idx].addr += n;
  }
  //解开
  if(p->vma_list[idx].len == 0) {
    fileclose(p->vma_list[idx].f);
    p->vma_list[idx].addr = MAXVA*2;
  }
  return 0;
}

uint64
sys_munmap(void)
{
  uint64 addr;
  int len;
  if(argaddr(0, &addr) < 0 ||
    argint(1, &len) < 0    || len < 1) {
    return -1;
  }
  return my_munmap(addr, len);
}

