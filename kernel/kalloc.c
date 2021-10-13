// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

#define PGINDEX(arr) (((PGROUNDDOWN((uint64)(arr)))-KERNBASE)/PGSIZE)
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.
struct spinlock cowLock;
char referenceCount[(PHYSTOP-KERNBASE)/PGSIZE];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&cowLock, "cowLock");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  acquire(&cowLock);
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  
  if(referenceCount[PGINDEX(pa)]-- > 1){
    release(&cowLock);
    return;
  }
  referenceCount[PGINDEX(pa)] = 0;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
  release(&cowLock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  if(r)
    referenceCount[PGINDEX(r)] = 1;
  return (void*)r;
}

int
isCow(uint64 va) {
  struct proc *p = myproc();
  if(va > p->sz)
    return 0;
  pte_t *pte = walk(p->pagetable, va, 0);
  //检查pte是否存在
  if(pte == 0 || !(*pte & PTE_V))
    return 0;
  //检查是否是cow 防止真只读片段被修改
  if((*pte & PTE_RSW_COW) == 0)
    return 0;
  return 1;
}

int
cowAlloc(uint64 va) {
  struct proc *p = myproc();
  pte_t *pte = walk(p->pagetable, va, 0);
  uint64 pa = PTE2PA(*pte);

  //reference == 1只需要改为可写
  *pte = (*pte | PTE_W) & ~PTE_RSW_COW ; //todo
  acquire(&cowLock);
  if(referenceCount[PGINDEX(pa)] == 1) {
    release(&cowLock);
    return 1;
  }
  
  //分配新空间并复制原内存
  uint64 *newPage = kalloc();
  if(newPage == 0){
    release(&cowLock);
    return 0;
  }
  memmove(newPage, (void*)pa, PGSIZE);
  //修改pte
  *pte = PTE_FLAGS(*pte) | PA2PTE(newPage);
  //修改原内存referenceCount
  referenceCount[PGINDEX(pa)]--;
  release(&cowLock);
  return 1;
}

void
increaseReferenceCount(uint64 pa) {
  acquire(&cowLock);
  referenceCount[PGINDEX(pa)]++;
  release(&cowLock);
}

void
decreaseReferenceCount(uint64 pa) {
  acquire(&cowLock);
  referenceCount[PGINDEX(pa)]--;
  release(&cowLock);
}

// uint64 getFreePage() {
//   int count = 0;
//   struct run *r = kmem.freelist;
//   while (r) {
//     r = r->next;
//     count++;
//   }
//   return count;
// }