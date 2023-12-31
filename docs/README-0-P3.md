# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 3。

#### 最新更改

[2023-11-09] 初步完成 Task 4。

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

  准备制造 Mailbox，将相关代码单独开了个文件`kernel/locking/mbox.c`。

#### [2023-11-05]

  初步实现了信箱（mailbox）但存在 Bug：当按下列顺序执行时：

```bash
exec mbox_client &
exec mbox_server &
kill 3
kill 2
```

会报出 Panic：“\*\*Panic: do_kill: Failed to del pcb 2 in queue 0x0\*\*”，初步判断是`do_kill()`中`ress_release_killed(pid);`的执行顺序不太合适。

  修复了上一个 bug，以及 mbox_client 发送计数不包括初始请求的问题。在多个发送端和一个接收端的情况下，可以先启动接收端、再启动多个发送端，可能看到接收端计数不等于发送端计数之和，这是因为有些信息还没有被接收；此时可以用`kill`终止掉发送端，待接收端停止计数（被阻塞）后它的计数值应当等于发送端的计数之和。注意，必须先启动接收端，否则终止掉发送端时，因为 mailbox 是发送端开的，该 mailbox 会被设为无效，所以接收端运行会出错。

#### [2023-11-06]

  多核 CPU，启动！初步为多核 CPU 做了准备，并先（似乎）实现了单核的兼容（即，不开启多核 CPU 似乎能正常运行）。

  成功启动 1 号 CPU（作为从核），但它啥也不干。注意，由于在收到核间中断之前，从核要停留在 BootLoader，所以在加载用户程序信息（`init_taskinfo()`）时不能将其覆盖了，而是暂时写在`Taskinfo_buffer = 0x52510000U`处，但这个初始化要在`malloc_init()`之前完成，因为这个地方属于`malloc-g.c`中的存储管理函数管理的空间。

  确认 1 号 CPU 上板能跑，仅打出字符串，且不影响单 0 号 CPU 的正常运行。

  目前能多核 CPU 运行了，但是存在严重 Bug，暂未查明原因，但已知问题出在`do_sleep()`、`check_sleeping()`以及`wake_up()`相关的东西上，复现方法是直接在 glush 里执行`sleep 2`命令。

#### [2023-11-07]

  对`panic_g()`做了些小改动，使得打印出当前 CPU ID。

  艰难`do_sleep()`调试进行中，目前 glush 做`sleep 2`不会有问题了，但是执行用户程序（例如`exec barrier &`）时用户程序做`sys_sleep()`仍然会出非法指令的错。

  调试进行中……

#### [2023-11-08]

  初步修复了`do_sleep()`的 Bug，错误在于切换到下一个进程时没有将其状态设置为`TASK_RUNNING`，导致两个 CPU 同时往一个进程上切的情况。但似乎目前系统仍然不太稳定，启动 multicore 测试的时候偶尔会报 illegal instruction。

  新增 glush 打印出 RUNNING 进程的 CPU 号的功能（`do_process_show()`的支持）。

  修复了上述不稳定问题导致的 illegal instruction，靠的是`load_task_img()`里的一个小补丁，当发现某个用户程序已经在运行的时候就不再重新从辅存加载它了。此外，支持了对单 CPU 的兼容，make 时指定变量`NCPU=1`即可禁用多 CPU 模式。已进行上板验证。下一步：处理跨核 `do_kill()`的问题。

  新增了对跨核`do_kill()`的支持，同时打了个小补丁：在 0 号 CPU 启动 glush 时获取内核锁以免被另一个 CPU 干扰。

  尝试用 -O2 选项编译，成功。make 时加上`DEBUG=0`即可禁用`-g`并开启`-O2`。下一步：绑核。

#### [2023-11-09]

  初步尝试设定进程的 CPU 亲和力，新增`do_taskset()`函数但还未封装系统调用。目前 -O2 编译可在 QEMU 上正常运行。

  新增`sys_taskset()`并在 glush 中支持，初步完成 Task 4 并在 QEMU 上用 -O2 编译验证，可正常运行。

#### [2023-11-12]

  填充`README.md`，Project 3 算是告一段落。