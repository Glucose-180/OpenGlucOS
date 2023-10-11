## Project 2 一些有趣的 Bug

### Part 1

#### 1. `tiny_libc/include/kernel.h`中的神奇定义

  在初步做了调度器并进行测试的时候，print1 一启动就报了非法指令的错误。通过施展 GDB 大法，最后把错误锁定在 print1 调用 `printf`时，调用跳转表`call_jmptab`（`tiny_libc/syscall.c`）时，参数`which`不对。我记得我已经在`include/os/kernel.h`中定义好了枚举：

```c
#define KERNEL_JMPTAB_BASE 0x51ffff00
typedef enum {
    CONSOLE_PUTSTR,
    CONSOLE_PUTCHAR,
    CONSOLE_GETCHAR,
    SD_READ,
    SD_WRITE,
    QEMU_LOGGING,
    SET_TIMER,
    READ_FDT,
    MOVE_CURSOR,
    PRINT,
    YIELD,
    MUTEX_INIT,
    MUTEX_ACQ,
    MUTEX_RELEASE,

    WRITE,
    REFLUSH,
    NUM_ENTRIES
} jmptab_idx_t;
```

  其中，所要调用的`screen_write`函数对应`WRITE`，即 14，而在调用`call_jmptab`时用户程序的`WRITE`竟然为 15。用编辑器的搜索功能，找到了定义的来处，在`tiny_libc/include/kernel.h`中：

```c
#define KERNEL_JMPTAB_BASE 0x51ffff00
typedef enum {
    CONSOLE_PUTSTR,
    CONSOLE_PUTCHAR,
    CONSOLE_GETCHAR,
    SD_READ,
    SD_WRITE,
    QEMU_LOGGING,
    SET_TIMER,
    READ_FDT,
    MOVE_CURSOR,
    PRINT,
    YIELD,
    MUTEX_INIT,
    MUTEX_ACQ,
    MUTEX_RELEASE,
    NUM_ENTRIES,
    WRITE,
    REFLUSH
} jmptab_idx_t;
```

  `WRITE`竟然在`NUM_ENTRIES`后面？这是什么情况？查了 Git 的历史版本，我并没有对它修改过。不管了，把它改成跟`include/os/kernel.h`里的一样再说。

  改了之后发现还不行。一拍脑袋，原来是`Makefile`里没有处理头文件修改的情况。解决方法自然是`make clean`。然后，就好了。

#### 2. 紧急修复`malloc-g.c`中的 Bug

  `malloc-g.c`是魔改的以前写的存储分配程序，这次突然发现它有个严重的 Bug，在初始化空闲分配区的`size`之前就尝试调用`foot_loc`获取其尾部标签的指针，这非常危险！

#### 3. 优化`malloc-g.c`中的释放函数`xfree_g`

  释放非动态分配的内存是非常危险的操作。为了使我的 GlucOS 尽可能稳定，需要让相关程序更加严密。为此修改了`malloc-g.c`中的分配与释放函数，在分配后让新分配的块的尾部`head`指针指向头部，这样`xfree_g`函数就可以检查待释放的块是否为动态分配的，否则就 panic。

```c
    /* Check whether the block is allocated by xmalloc_g */
    if (((int64_t)P & (ADDR_ALIGN - 1)) != 0 ||
        ((int64_t)f & (ADDR_ALIGN - 1)) != 0 ||
        f->head != h)
        panic_g("kfree_g: Addr 0x%lx is not allocated by xmalloc_g\n", (int64_t)P);
```

### Part 2

#### 1. 用户程序的特权级

  在做 Task 3 过程中，我首先尝试将已有的用跳转表实现的伪系统调用改造成了真的系统调用。根据 RISC-V 体系结构异常处理的有关知识，我设置了`stvec`等控制寄存器，也调用了`enable_interrupt()`以及`enable_preempt()`等做初始化，异常处理的函数也写好了。然而在用户程序触发系统调用的时候，却马上报出了错误，屏幕上打印出了奇怪的看不懂的信息。还是启用 GDB 大法，我将断点打在用户程序中，检查发现`stvec`寄存器已经正确设置；将断点打在异常处理函数`exception_handler_entry()`的入口处，却发现当用户程序进行系统调用时根本没有触发这一断点。也就是说，`ecall`指令并没有使 CPU 跳到`stvec`所指函数处去执行。这让我不知所措。

  通过查阅 RISC-V 手册、询问老师以及自己反思，我想到了一个问题：我们的 CPU 在上电时应当是处于 Machine 模式下，GlucOS 启动时会切换到 Supervisor 模式。然而，GlucOS 启动用户程序的方法是加载到主存然后直接跳转，也就是说我们的用户程序是在 Supervisor 模式下运行的。在 Supervisor 模式下执行`ecall`指令会发生什么呢？那不就陷入 Machine 模式下了么，`stvec`自然也就不好用了。所以，我需要将用户程序运行在 User 模式下。

  为了让 GlucOS 在运行用户程序时切换特权级到 User，我选用的方法是在新进程创建时将其 PCB 的`trapframe`设为 `NULL`，一旦它触发过系统调用该指针就不会为`NULL`，并且保证再也不会被改回`NULL`。这样，它就成了新进程的标志。在`switch_to()`函数中多加一层判断，对于非新进程使用常规的`jr`指令跳转过去，而对于新进程则将`ra`写入`sepc`后使用`sret`指令跳转，从而使新进程第一次运行时能处于 User 模式。

```assembly
    /* Now $t0 equals current_running */
    ld	t1, PCB_TRAPFRAME(t0)
    beq	zero, t1, New_task
    /*
    * c.r.->trapframe being NULL means it is a new process.
    * So sret is used to change to User Mode from Supervisor Mode.
    */
    jr	ra
New_task:
    csrw	CSR_SEPC, ra
    sret
```

  在修复了这个错误后，我突然发现在我们任务书的任务 3 的“注意事项”里第一条就是：

>   了解 RISC-V 下 ecall 指令的作用，该指令会触发系统调用例外。RISC-V 在所有 特权级下都用 ecall 执行系统调用。Supervisor 态 ecall 会触发 machine 态的例外， user 态的 ecall 会触发supervisor 态的中断。所以大家务必注意，要让 USER 模式的进程运行在用户态。

  啊好吧！都怪我没有读完任务书就开始做实验。不过，在没有看到这条注意事项的条件下遇到错误自己调试得出这个结论，也算是一次有效的训练。



