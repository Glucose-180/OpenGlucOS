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
	 ...
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

#### 2. `sstatus`中的 SPIE 位不能正确设置

  虽然 Task 3 已经测试成功了，但为了理解 RISC-V 体系结构的异常处理过程，我还是启动了 GDB 大法来跟踪研究一次`ecall`系统调用中的各 CSR 的变化情况，然后就发现了奇怪的问题：尽管我在内核启动时调用了`enable_interrupt`将`sstatus`的 SIE 置`1`了，在发生系统调用时用 GDB 显示`sstatus`的值却看到其 SPIE 不为`1`。然而根据 RISC-V 手册中的说法，发生异常时硬件会将 SPIE 设为与 SIE 相同并将 SIE 置`0`。理论上讲，我可以用 GDB 一直跟踪`sstatus`的变化，但遗憾的是一旦进入了 User Mode，GDB 就看不到`sstatus`的值了，这给这个问题的解决造成了一定的麻烦。

  后来我意识到一个问题，那就是我在异常返回恢复现场的`RESTORE_CONTEXT`中没有恢复`sstatus`。但在我修复了这个问题之后，原有的问题仍然没有解决。就在我一筹莫展的时候，2023 年 10 月 13 日的凌晨 4 点左右，我从睡眠态自动唤醒了（虽然这在计算机中通常是不允许的，睡眠态的进程只能被其它进程或操作系统唤醒），前一天晚上睡觉前的上下文立即恢复。在我重新进入睡眠态之前我突然意识到第一个用户进程启动时使用了`sret`指令（前面 1 刚说的），那么这时候 SIE 是不是就会被设为 SPIE 呢？可是 SPIE 为`0`啊！所以第一个用户进程启动后 SIE 就已经是`0`了，那么当它执行系统调用的时候自然 SPIE 也是`0`了。想到这里我赶紧保存了上下文之后再进入睡眠态。终于在清晨，我修复了这个问题，具体方法就是在`switch_to()`函数中启动第一个用户进程的`sret`指令之前，将 SPIE 设为与 SIE 相同。

```assembly
	...
New_task:
    csrw	CSR_SEPC, ra

    /*
    * Make SPIE of $sstatus equal SIE of $sstatus
    * before using sret to run a new task, so that
    * SIE will not be modified.
    */
    csrr	t2, CSR_SSTATUS
    and	t1, t2, SR_SIE
    slli	t1, t1, 4
    and	t2, t2, ~SR_SPIE
    or	t2, t2, t1
    csrw	CSR_SSTATUS, t2

    sret
```

#### 3. 系统调用的误触发

  按照任务书中的说法，我们要使用`loadbootd`命令来启动内核，使得某种检查功能启用，当用户程序试图访问内核的主存空间时会触发例外。我编写了一个新的用户程序叫`GlucOStest`用于测试我的操作系统（它具备测试访问内核是否会报例外、执行系统调用能否正确恢复现场、`printf()`函数能否正常打印有符号数值等功能），其源代码在`test/test_project2/GlucOStest.c`。在`loadbootd`的条件下，尝试访问内核确实会报例外。然而在`loadboot`下，尝试访问内核却触发了我内核的`invalid_syscall()`中的 PANIC。

  所谓 PANIC，是指我为了使自己的操作系统在开发过程中更稳定、减少安全隐患、降低调试的难度，在内核的代码中插入了大量的检查代码，一旦发现某操作的执行发生了异常则“**紧急制动**”，调用`panic_g()`函数（目前放在`libs/glucose.c`中）打印报错信息并立即使系统停下，避免程序跑飞而大大增加调试难度。其思想很简单，基本上与`assert()`一样，但**在缩小错误范围、降低调试难度上已经发挥了非常显著的作用**。`invalid_syscall()`的 PANIC，则是处理系统调用时遇到了无效的系统调用号导致的。

  `GlucOStest`尝试访问内核的做法是直接通过跳转表调用内核函数`port_write_ch()`。为什么这会导致无效系统调用呢？我启动 GDB 大法，查看`port_write_ch()`调用的`BIOS_FUNC_ENTRY`（在`arch/riscv/bios/common.c`）处的指令……啊好吧，那没事了。

```bash
(gdb) x/i 0x50150000
   0x50150000:	ecall
```

#### 4. 用户程序被“调包”了？

  在 Task 4 中，任务书要求需要注释掉用户程序源代码中所有的`sys_yield()`以测试抢占式调度。我用编译预处理（宏定义）的方法解决的，并且先尝试修改了`print1.c`、`print2.c`和`fly.c`来测试。修改后，我`make all`然后运行。然而当我在 GlucOS 中同时启动上述 3 个程序时却出现了奇怪的现象：print1 不能运行（屏幕最上面一行没有打印出字符）。启动 GDB 大法，发现当定时器中断调度到第一个进程（按理来说应该是 print1）时，执行的却是 fly 的效果。

  发生什么事了？我重启 GlucOS，单独启动它们的每一个程序，运行都是正常的，甚至同时启动两个也不会出问题，这说明用户程序的编译是正常的，并且正确装入了镜像。我调试了很长时间，内核的调度器也检查了，都没有发现问题。当时已经是 10 月 19 日的深夜，马上要 20 日了，我撑不了只好保存实验的上下文然后进入睡眠态。

  第二天早上我继续调试，我希望看一看`make all`时的编译命令，于是我先`make clean`然后`make all -n`，没有看出异常。这时我重新编译运行，好，**问题竟然消失了**？

​    原来，问题并不出在我的设计上，而是出在 make 上。`Makefile`里关于用户程序的编译命令的定义是这样的：

```makefile
$(DIR_BUILD)/%: $(DIR_TEST_PROJ)/%.c $(OBJ_CRT0) $(LIB_TINYC) riscv.lds
	$(CC) $(USER_CFLAGS) -o $@ $(OBJ_CRT0) $< $(USER_LDFLAGS) -Wl,--defsym=TEXT_START=$(USER_ENTRYPOINT) -T riscv.lds
	$(eval USER_ENTRYPOINT := $(shell python3 -c "print(hex(int('$(USER_ENTRYPOINT)', 16) + int('0x10000', 16)))"))
```

  

  别忘了，用户程序的入口点`TEXT_START`是由`USER_ENTRYPOINT`指定的，而这个值每编译一个程序就会递增 0x10000。这样，当我仅修改了所有用户程序中的某几个的源代码时，由于未修改的源代码不需要重新编译，所以`make all`时会对新修改的几个源代码重新编译重新计算入口点，这样就难免和已经编译好的用户程序的入口点发生冲突（注意，我并不是一次性修改了`print1.c`、`print2.c`和`fly.c`然后才编译的，而是先修改了一个编译后又改的另外两个）。而 GlucOS 内核加载用户程序时会根据镜像中保存的入口点加载到主存的相应位置，因此运行多个用户程序时如果它们的入口点冲突了，就会导致后加载者覆盖先加载者，从而出现用户程序被“调包”的现象。

  ~~这个错误说明，解决问题的有效办法终究还得是重启、重装、重买。~~

