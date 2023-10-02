## Project 2 Part 1 一些有趣的 Bug

#### 1. `tiny_libc/include/kernel.h`中的神奇定义

​	在初步做了调度器并进行测试的时候，print1 一启动就报了非法指令的错误。通过施展 GDB 大法，最后把错误锁定在 print1 调用 `printf`时，调用跳转表`call_jmptab`（`tiny_libc/syscall.c`）时，参数`which`不对。我记得我已经在`include/os/kernel.h`中定义好了枚举：

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

​	`WRITE`竟然在`NUM_ENTRIES`后面？这是什么情况？查了 Git 的历史版本，我并没有对它修改过。不管了，把它改成跟`include/os/kernel.h`里的一样再说。

​	改了之后发现还不行。一拍脑袋，原来是`Makefile`里没有处理头文件修改的情况。解决方法自然是`make clean`。然后，就好了。

#### 2. 紧急修复`malloc-g.c`中的 Bug

​	`malloc-g.c`是魔改的以前写的存储分配程序，这次突然发现它有个严重的 Bug，在初始化空闲分配区的`size`之前就尝试调用`foot_loc`获取其尾部标签的指针，这非常危险！

#### 3. 优化`malloc-g.c`中的释放函数`xfree_g`

​	释放非动态分配的内存是非常危险的操作。为了使我的 GlucOS 尽可能稳定，需要让相关程序更加严密。为此修改了`malloc-g.c`中的分配与释放函数，在分配后让新分配的块的尾部`head`指针指向头部，这样`xfree_g`函数就可以检查待释放的块是否为动态分配的，否则就 panic。

```c
	/* Check whether the block is allocated by xmalloc_g */
	if (((int64_t)P & (ADDR_ALIGN - 1)) != 0 ||
		((int64_t)f & (ADDR_ALIGN - 1)) != 0 ||
		f->head != h)
		panic_g("kfree_g: Addr 0x%lx is not allocated by xmalloc_g\n", (int64_t)P);
```

