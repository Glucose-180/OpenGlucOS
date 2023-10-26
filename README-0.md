# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 2。

#### 最新更改

[2023-10-26] 发现`ufree_g()`存在严重问题。

#### 可做的优化

  给用户程序提供`getchar()`等函数；优化修复 createimage。

#### [2023-09-29]

​	正式开始 P2-T1。

#### [2023-09-30]

​	造了内存分配、PCB 链表操作等轮子，还没有做 `do_scheduler`。修复了编译错误。

#### [2023-10-01]

​	初步完成了 Task 1。上板也成功了。

#### [2023-10-02]

​	修复了创建进程（`create_proc`）时初始化 PCB 的`user_sp`不对的问题，它应该等于调用`umalloc_g`动态分配的地址还得再加上栈的大小（`Ustack_size`）。此外，新版的`xmalloc_g`函数会让新分配的块的尾部`head`指针指向头部，这样`xfree_g`函数就可以检查待释放的块是否为动态分配的，否则就 panic。

#### [2023-10-04]

​	使用新增的`split()`函数优化了命令的输入方法，可以用`&`连接多个命令。此外，支持`exit`命令。

#### [2023-10-07]

​	初步完成了 Task2。上板也成功了。

#### [2023-10-08]

​	修改了 Task2，使用`do_block()`与`do_unblock()`。使用`vprintk()`修复了`panic_g()`读取参数异常的问题。初始化了用户程序的内核栈`kernel_sp`，以备后面的任务使用。

​	P2-Task3，启动！

#### [2023-10-09]

​	初步完成了系统调用。

#### [2023-10-10]

​	新增`bios_getchar()`作为系统调用 21，并新增了`genshin.c`用于测试系统例外处理的功能。

​	修改了`sched.c`中的相关代码，使得添加用户程序时，会从移除代表内核`main()`函数的 0 号 PCB（PID 为 0）使之不参与调度。

#### [2023-10-11]

​	首次完成了 Task 3，修复了`glucose.c`中`panic_g()`使用`printk()`存在的问题（当`current_running`无效时，无法正确打印错误信息），修复了`printf/k`不能打印负数的问题。

​	**注意：**再次修改了调度器，现在又不允许`current_running`为`NULL`了，添加用户程序时不再移除内核所属的 0 号进程。因为，`current_running`为`NULL`可能在遇到中断时导致灾难，形象地说就是“没有人来处理中断”了。此外，今后做中断实验时，**请留意`entry.S`中`exception_handler_entry`开头的注释**！

#### [2023-10-12]

​	修改了`entry.S `的`RESOTRE_CONTEXT`，在恢复上下文时把`sstatus`也恢复。但发现即使这样，在一次系统调用返回后`sstatus`的 SIE 位仍然不能恢复为`1`。为什么？需要手动恢复吗？

#### [2023-10-13]

​	修复了在用户态执行`ecall`系统调用时`sstatus`的 SPIE 位不为`1`的问题（详见`~Record`中的记录）。修改了`Makefile`，使之在`make gdb`时加上`-q`选项让它安静一点。修复了`tiny_libc/syscall.c`中`Ignore`（原来叫`IGNORE`）变量未使用导致的 warning。

#### [2023-10-16]

  修复了多处问题：

  `SAVE_CONTEXT`中，把`t0`临时保存到`sscratch`中，而不是用户栈。

  系统启动初始化`init_exception()`时，将`sstatus`的`SPIE`置`1`，`SPP`置`0`，而不是在`switch_to()`里做这个工作。考虑使用 XV6 的`riscv.h`文件。

#### [2023-10-17]

  正式开始 Task 4。将`init_exception()`中对`enable_interrupt()`的调用改到`setup_exception()`中。修改了多个文件，铺好了进入时钟中断的路。已经可以进入时钟中断了，但在`handle_irq_timer()`里什么也不做。但好像只要执行到`wfi`就会不断地进入时钟中断，莫非只要不去 reset timer，这个中断请求就会始终存在？

  经过测试了，确实是这样。如果在`handle_irq_timer()`里 reset timer，这个中断请求就会消失直到下一次时钟中断到来。

#### [2023-10-18]

  查阅 RISC-V 指令集手册（`riscv-privileged-20211203.pdf`）4.1.3，可以看到有这么一段话：

> Bits `sip`.STIP and `sie`.STIE are the interrupt-pending and interrupt-enable bits for supervisor level timer interrupts. If implemented, STIP is read-only in `sip`, and is set and cleared by the execution environment.

  原来`SIP`的定时器中断标志 STIP 是只读的，它只能被执行环境改变，这就不难解释前一天观察到的现象了，估计是只有当调用 BIOS 做 reset timer 后它才会被清零。

  关于定时器的相关内容可参见上述指令集手册的 3.2.1。

  尝试抢占式调度失败了。任务书的注意事项里有这么一句话：

> 关于如何正确的开始一个任务的第一次调度，在抢占调度下是和非抢占调度是不同的，希望大家仔细思考如何在抢占调度模式下对一个任务发起第一次调度。

  不同之处在于，抢占模式下定时器中断使`sstatus`的 SPP 为`1`，因此如果在`handle_irq_timer()`里直接调`do_scheduler()`的话，会导致第一个用户进程启动时由`sret`进入 Supervisor Mode，这是错误的，因为用户进程应该运行在 User Mode。

#### [2023-10-19]

  初步实现抢占式调度，但逻辑有点乱七八糟，还需要修理一下。

  修复修改了多处问题：`RESOTRE_CONTEXT`恢复现场时，将`sepc`、`sstatus`、`scause`都恢复；用户程序的初次运行不再从`switch_to`中`sret`，而是从中`jr ra`到`ret_from_exception()`（需要在`init_pcb_stack`中提前准备一个假的 trapframe；`panic_g()`时，要关中断；清除了很多历史遗留问题……

 23:54 遇到离奇 Bug！当启动 3 个程序（例如`print1 & print2 & fly`）时，第一个程序不能正常调度，却相当于启动了`fly & print2 & fly`！

#### [2023-10-20]

  昨晚的离奇 Bug 在`make clean`后就好了，估计是只修改了部分源文件，导致`make`的时候自动计算的入口地址和别的程序冲突了。正在进行`YIELD_EN`宏定义的添加，以控制用户程序是否主动交出控制权。目前已给`fly.c`、`print1.c`和`print2.c`添加了这个控制。

  给所有用户程序添加了`YIELD_EN`的控制。抢占式调度初步测试成功了。

  修改了`Makefile`，使得在`make minicom`时延迟一段时间以跳过开发板输出的长长的信息。

  在 PCB 中添加了内核栈和用户栈的地址（分配的起始地址，用于今后做回收）；修复了一个暂时影响不大的问题：初始化进程 PCB 时没有对栈指针做 16 字节对齐。

  修改了内核栈和内核动态分配空间的大小：内核动态分配空间（由`kmalloc_g()`分配）和用户动态分配空间均为16 MiB，地址范围分别为 0x50500100~0x51500100 和 0x52500000~0x53500000；每个进程的内核栈和用户栈的默认大小均为 4 KiB。

#### [2023-10-23]

  准备 C Core Task 5，先处理一个历史遗留问题：去掉多余的类型定义`clist_node_t`。

#### [2023-10-24]

  给`pcb_list_g.c`中对 PCB 链表的操作函数加上`lpcb_`前缀，以便与即将加入的 TCB 链表（已经加了空文件）操作区分。

  新增了 TCB 单链表的操作支持，但还没有经过测试。

  新增了线程操作（在`thread.c`），但未经过测试。如果在支持多线程后，需要修改所有进程上下文切换`switch_to()`的调用，在调用时判断当前进程的线程是否是子线程，如果是的话需要切换到子线程的上下文。

  在给徐畅杰调 Task 5 时发现，我们的工具`createimage.c`可能存在严重缺陷，它不能正确处理某一个 ELF 文件有多个大小不为零的 Segments 的情况，因为它在每次写一个 Segment 进镜像后是紧接着写下一个。当两个相邻 Segments 在 ELF 文件中因为对齐等要求并非紧密排列（例如，有 4 个字节的 padding）时，会导致在镜像被加载到主存后后面的 Segment 的地址不正确。

#### [2023-10-25]

   Task 5 进行中。新增多线程支持，尚未经过测试。

  经过十分痛苦的调试，终于初步实现了多线程支持。但是留下了大量的烂摊子待收拾。

#### [2023-10-26]

  收拾了大量烂摊子。发现存储释放`ufree_g()`存在问题，正在修复。最后查出来是因为进入子线程函数时，它会修改地址比`sp`更高的部分的内容（栈帧约定），解决方法是初始化的`sp`比动态分配的栈空间的底部再低 512 B。