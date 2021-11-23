# Mit6.S081-Note

这是我自学完成的第一门公开课

lab通关时间: 2021.9.10 ~ 2021.11.8



作业代码:  切换到对应的 branch

课程:

* [课程主页](https://pdos.csail.mit.edu/6.S081/2020/schedule.html)
* [lecture中文文档](https://mit-public-courses-cn-translatio.gitbook.io/mit6-s081/)
* [xv6手册](./Resources/XV6-Chinese-2020.pdf)

相关资源: 

* [gdb的使用](./Resources/gdb/README.md)
* [其他关于risc-v以及xv6的资料](./Resources/riscv)

推荐的博主博客:

* [Miigon的6.S081 lab笔记](https://0xffff.one/d/1085-mit6-s081-operating-system-engineering-cao-zuo-xi-tong-she-ji-ke-cheng-jie-shao)
* [关于xv6源码的思维导图以及代码解析](https://juejin.cn/post/7024713350567165965)

逻辑梳理:

* 用户空间, 用户程序
  * system call使用, 命令行参数读取
  * pipe, fork, io等
* 内核
  * 隔离 防御
  * user / kernel mode
  * 微内核  宏内核
* trap
  * trap的分类: 驱动中断, 系统调用(ecall), 定时器中断
  * 硬件层面对中断的支持: PLIC
  * trapframe, trampoline
* 虚拟内存
  * 页表机制
  * 应用: lazy, mmap, copy on write, demand paging(改exec)
  * 内存分配, 释放, 映射
* 进程的生命周期
  * 进程的几种状态free , runable, sleep, zoom
  * 进程对象维护的数据
  * 进程的内存模型(text, stack....)
  * 所有进程的初始化, 单个进程创建与释放
  * 进程切换过程, 进程调度器(值得注意, 交换了锁)
  * 进程之间的通信(buffer, 生产者-消费者)
* 锁
  * 常见的并发问题: 竞态(race condition), 死锁, 无法醒来的进程(lose wake)
  * 锁的种类: 自旋(互斥)锁, 信号量, 睡眠锁(拿到被其他进程持有的锁进入睡眠), 屏障(barrier)
  * 问题的解决: 按规定顺序获得锁, 
* 文件系统
  * 磁盘的基本概念: sector(512), block(1024), inode(64)
  * 磁盘上的模型(boot, super block,log, inode, bitmap, data block)
  * 磁盘上inode的结构, 目录和entry(16)
  * 日志
    * data block -- log block -- block buffer -- inode -- file
    * block首先得被加载buffer中, 读写都需要获得buffer
    * log的结构 header , log block
    * log的提交过程 commit - > install - > clean header
  * ext3
    * 如何提升性能: 异步, 批量, 并发
    * 执行过程

