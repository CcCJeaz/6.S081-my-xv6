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

void  init_kfree(void *pa, int cid);
void init_freerange(void *pa_start, void *pa_end, int id);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];

#define ORIGIN_SIZE ((PHYSTOP - PGROUNDUP((uint64)end)) / NCPU)
void
kinit()
{
  for(int i=0; i<NCPU; i++) {
    initlock(&kmems[i].lock, "kmem");
    char *bottome = (char *)PGROUNDUP((uint64)end + i*ORIGIN_SIZE);
    char *top = bottome + ORIGIN_SIZE;
    init_freerange(bottome, top, i);
  }
}


void
init_freerange(void *pa_start, void *pa_end, int cid)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    init_kfree(p, cid);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
init_kfree(void *pa, int cid)
{
  struct run *r;
  
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  r->next = kmems[cid].freelist;
  kmems[cid].freelist = r;

}

// void
// freerange(void *pa_start, void *pa_end)
// {
//   char *p;
//   p = (char*)PGROUNDUP((uint64)pa_start);
//   for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
//     kfree(p);
// }


// int countFree() {
//   int count = 0;
//   for(int i=0; i<NCPU; i++) {
//     struct run *t = kmems[i].freelist;
//     while (t) {
//       count ++;
//       t = t->next;
//     }
//   }
//   printf("num: %d\n", count);
//   return count;
// }

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  push_off();
  int cid = cpuid();
  pop_off();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[cid].lock);
  r->next = kmems[cid].freelist;
  kmems[cid].freelist = r;
  release(&kmems[cid].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off();
  int cid = cpuid();
  pop_off();

  acquire(&kmems[cid].lock);
  r = kmems[cid].freelist;
  if(r)
    kmems[cid].freelist = r->next;
  release(&kmems[cid].lock);
  
  if(!r) {
    for(int i=NCPU-1; i>=0; i--) {
      if(i == cid)
        continue;
      acquire(&kmems[i].lock);
      if(!kmems[i].freelist){
        release(&kmems[i].lock);
        continue;
      }
      r = kmems[i].freelist;
      kmems[i].freelist = r->next;
      release(&kmems[i].lock);
      break;
    }
  }
  

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  
  return (void*)r;
}
