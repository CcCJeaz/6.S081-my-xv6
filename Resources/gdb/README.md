gdb一些好用的网站

[每行打印一个结构体成员 | 100个gdb小技巧](https://wizardforcel.gitbooks.io/100-gdb-tips/content/set-print-pretty-on.html)

[Top (Debugging with GDB) (官网)](https://sourceware.org/gdb/onlinedocs/gdb/index.html)

一些基本操作

窗口
layout src
layout asm
layout split
layout reg

tui reg general
tui reg float
tui reg system
tui reg next

focus next/prev

调试小技巧
until 运行至调出循环体

finish 完成函数返回
where 查看执行过程

打印
`print/<f> <expr>`
操作符:` @   ::   {<type>} <addr>`
`<f>` : x, d, u, o, t, a, c, f

`x/<n/f/u> <addr>` 查看地址
n--显示地址的长度
f---显示格式同上
u---表示每次偏移多少
  b单字节, h双字节, w四字节, g八字节

`display/<fmt> <expr> `自动显示

$ 寄存器
