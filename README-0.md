# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 3。

#### 最新更改

[2023-11-04] 修复了已经退出的进程又被切换回来的问题。

#### 可做的优化



#### [2023-10-27]

  初始化 Project 3 的仓库。

  修复了编译错误。注意：本实验要实现的 shell 的源代码搬到了在`test/test_project3`文件夹中，叫`glush.c`，原本的源代码在`test/shell.c`。

  修改了部分代码的缩进为 Tab。

  为 glush（自制 shell 终端）提供必要的库函数（`getchar()`、`getline()`等），并实现命令回显。

  制造出`sys_ps()`、`sys_clear()`并在 glush 中支持。

  初步实现`sys_exec()`，但是函数原型与初始代码不一样，我实现的不需要`argc`这个参数。下面，可以考虑删除内核的假命令行了，而让 glush 自启动即可。

#### [2023-10-28]

  修复了`sys_exec()`的原型，增加了`argc`参数，具体用法为：若`argc`大于最大允许的数值（`ARGC_MAX`，目前为 8）则报错；若`argc < 0`，则根据`argv[]`自动确定`argc`；若`argv`为`NULL`，则`argc`自动为 0；若`argc`比`argv[]`中实际参数数量大，则以实际数量来设置`argc`；若`argc`比`argv[]`中实际参数数量小，则保持`argc`，多余的参数将被忽略。初步测试成功。

  修复了`do_process_show()`，补充了对互斥锁阻塞队列的扫描。

  将宏定义`TASK_MAXNUM`换为`UTASK_MAX`，表示最大支持的用户程序数（当前为 32）；将宏定义`NUM_MAX_TASK`换为`UPROC_MAX`，表示 GlucOS 最大支持的用户进程数（不含 PID 0），当前定义为 16。

  新增了`pcb-table.c`用于维护一个表`pcb_table[]`，它保存当前所有进程的 PCB 的指针以便查找。**每次新增（创建进程）或删除（终止进程）PCB 时， 需要调用其中的函数来更新这个表。**

  临时修复了`syscall.c`中未实现的函数以避免编译器报警告。

  首次实现了`sys_kill()`：注，修改了 PCB，新增了`phead`成员，它指向当前 PCB 所属的队列的头指针，例如，`ready_queue`中的 PCB 的`phead`应该等于`&ready_queue`。时间仓促，尚未充分测试。下一步：`sys_exit()`，可用`sys_kill()`实现。

#### [2023-10-29]

  首次使用`printl()`，将其包装成`writelog()`（在`glucose.c`）供内核使用，封装为系统调用`sys_ulog()`供用户使用，给用户提供`printl()`，它会对格式做处理后调用`sys_ulog()`，因为`sys_ulog()`只接受纯字符串参数而不做格式处理。新增`sys_kprint_avail_table()`系统调用，用于在日志中打印内核空闲链表信息；经过检查，在多次创建、终止进程后空闲链表可以复原。在 glush 中添加日志功能，可以通过`argv[3]`指定是否自动打印日志，即使不自动打印也可以用`ulog`命令手动打印。使用 QEMU 时，日志会保存在`~/OSLab-RISC-V/oslab-log.txt`，使用`tail -f`可动态查看。后来 QEMU 和 FPGA 板卡的日志分别被指定为了`./glucos-qemu.log`和`./glucos-fpga.log`，但在使用 FPGA 板卡需先要输入`printlon`命令再`loadbootd`。

首次实现进程退出`sys_exit()`，并在`crt0.S`中自动调用。

  修改了`do_mutex_acquire()`以及`do_block()`，将 reschedule 的操作集成到`do_block()`中。初步实现`sys_waitpid()`并在 glush 应用。在 glush 新增`sleep`命令。实现`sys_getpid()`。现在。可以在 glush 运行 waitpid 了（`exec waitpid`）。

  尝试在出现无效系统调用以及触发 DASICS 时报 Segmentation fault 并终止进程，在触发未知异常时 panic。

  优化屏幕驱动`screen.c`的换行支持，同时防止输入或退格时缓冲区溢出。

  初步实现自动滚屏，进程通过`sys_set_cylim()`可设置本进程的滚屏范围。

#### [2023-10-31]

  修复潜在 Bug：初始化第一个用户程序（`init_pcb_stack()`）时全局中断处于开启状态（`sstatus`的 SIE 为 1），直接将此时的`sstatus`初始化到用户程序的假现场（`trapframe->sstatus`）会导致在进入用户程序时，`RESTORE_CONTEXT`之后、`sret`之前就开启了中断。如果定时器中断的间隔很小，就可能导致在`sret`之前就去响应定时器中断，从而导致严重错误。因此在初始化假现场时，需要把当前`sstatus`与上`~SR_SIE`后再写到`trapframe->sstatus`中。此外，还要确保 SPP 为 0 以确保`sret`后用户程序运行在 User Mode。

#### [2023-11-02]

  新增 glush 对`&&`和`&`的支持。其中`&&`连接多条命令，在`main()`函数中处理并将`&&`连接的多条命令**依次**（前一条完成后再执行下一条）送给相应的处理函数（目前仅有`try_syscall()`）`&`连接一条命令中的多个部分，在`try_syscall()`中可以处理，它依次执行`&`连接的多条系统调用，对于`exec`命令，若后面有`&`则放到后台执行。

  新增 GlucOS 启动时自动运行 glush；新增区域清屏系统调用`sys_rclear()`且 glush 支持运行时通过`range`命令调节终端显示范围。

#### [2023-11-03]

  新增对宏定义`DEBUG_EN`的支持，在 make 时选定`DEBUG=1`（默认即可）可在编译时定义`DEBUG_EN=1`。当它被定义为非零值时，系统内核以及 glush 会自动写日志，并且在启动时也会打印内核信息。

  添加了自旋锁机制，并用在了互斥锁上。对互斥锁以及今后要实现的信号量、屏障等资源，均使用理论课上讲的编程习语，以考虑后面实现双核的情况：

```c++
Apply_for_a_resource(r)
{
	spin_lock_acquire(&r.slock);
    while (r is not available)
        /*
         * Block this process in r.queue, release r.lock,
         * and then reschedule (switch to another process).
         * After being unblocked, reacquire the spin lock.
         */
        do_block(r.queue, &r.slock);
    Occupy resource r;
    spin_lock_release(&r.slock);
}

Release_a_resource(r)
{
    spin_lock_acquire(&r.slock);
    Release resource r;
    if (r.queue != EMPTY)
        /* Unblock a process from r.queue */
        do_unblock(r.queue);
    spin_lock_release(&r.slock).
}
```

此外，优化了`lock1/2.c`以及`ready_to_exit.c`，使获得锁后调用`sys_sleep()`睡几秒钟以获得更好的视觉效果。

  初步实现了信号量（Semaphore）相关的功能，可以运行 semaphore 测试程序，但似乎还不太稳定，特别是涉及到 kill 的时候。如果在它们运行过程中终止掉 semaphore，则所有 sema_consumer 会立即退出（所用的信号量会失效，正在阻塞的消费者会立即唤醒，并且所有 P 操作也都失败，但 sema_consumer 并不会检测到错误返回值从而继续运行），sema_producer 会依次退出；如果终止掉某个未完成的 sema_producer，则会有 sema_consumer 不能退出而是始终沉睡，因为信号量的 V 操作不够了。

#### [2023-11-04]

  初步实现了屏障（Barrier），但还是存在鲁棒性问题，并发执行两个`exec barrier &`时可能出现“\*\*Panic: do_kill: proc 4 is still running after killed\*\*”的错误。

  修复了上面的问题：原因是在`do_sleep()`的时候，直接选择了`current_running->next`作为当前进程睡觉后的下一个进程，而没有检查它是否是`TASK_READY`的，导致有可能把已经`TASK_EXITED`的进程重新切换（`switch_to`）过来。

