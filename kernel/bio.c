// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define BKT_IDX(blockno) ((blockno)%NBUCKET)

struct bucket {
  struct spinlock lock;
  struct buf head;
  int freeNum;
};

struct {
  struct buf buf[NBUF];
  struct bucket bucket[NBUCKET];
  struct spinlock lock;
} bcache;

void
binit(void)
{
  initlock(&bcache.lock, "get");
  // Create linked list of buffers
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.bucket[i].lock, "bcache.bucket");
  }

  for (int i = 0; i < NBUF; i++) {
    if(i != NBUF-1)
      bcache.buf[i].next = &bcache.buf[i+1];
    initsleeplock(&bcache.buf[i].lock, "buffer");
  }
  bcache.bucket[0].head.next = bcache.buf;
  bcache.bucket[0].freeNum = NBUF;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct bucket *bkt = &bcache.bucket[BKT_IDX(blockno)];
  struct buf *b;

  acquire(&bkt->lock);

  // Is the block already cached?
  for(b = bkt->head.next; b != 0; b = b->next) {
    if(b->dev == dev && b->blockno == blockno) {
      if(b->refcnt == 0)
        bkt->freeNum--;
      b->refcnt++;
      release(&bkt->lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.


  for(b = bkt->head.next; b != 0 && bkt->freeNum>0; b = b->next) {
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      bkt->freeNum--;
      release(&bkt->lock);
      acquiresleep(&b->lock);
      return b;
    }
    
  }

  release(&bkt->lock);


  struct bucket *dst_bkt = bkt;

  struct bucket *src_bkt;
  
  for(src_bkt = bcache.bucket; src_bkt < bcache.bucket+NBUCKET; src_bkt++) {
    if(src_bkt == dst_bkt)
      continue;
    acquire(&src_bkt->lock);
    for(b = &src_bkt->head; b->next != 0 && src_bkt->freeNum>0; b=b->next) {
      
      if(b->next->refcnt == 0) {
        struct buf *freebuf = b->next;
        b->next = freebuf->next;
        src_bkt->freeNum--;
        release(&src_bkt->lock);
        
        acquire(&dst_bkt->lock);
        freebuf->next = dst_bkt->head.next;
        dst_bkt->head.next = freebuf;
        release(&dst_bkt->lock);

        freebuf->dev = dev;
        freebuf->blockno = blockno;
        freebuf->valid = 0;
        freebuf->refcnt = 1;
        acquiresleep(&freebuf->lock);
        return freebuf;
      }
    }
    release(&src_bkt->lock);
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  struct bucket *bkt = &bcache.bucket[BKT_IDX(b->blockno)];
  acquire(&bkt->lock);
  b->refcnt--;
  if(b->refcnt == 0) {
    bkt->freeNum++;
  }
  release(&bkt->lock);
}

void
bpin(struct buf *b) {
  struct bucket *bkt = &bcache.bucket[BKT_IDX(b->blockno)];
  acquire(&bkt->lock);
  if(b->refcnt == 0) 
    bkt->freeNum--;
  b->refcnt++;
  release(&bkt->lock);
}

void
bunpin(struct buf *b) {
  struct bucket *bkt = &bcache.bucket[BKT_IDX(b->blockno)];
  acquire(&bkt->lock);
  b->refcnt--;
  if(b->refcnt == 0) 
    bkt->freeNum++;
  release(&bkt->lock);
}


