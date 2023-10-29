# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 3。

#### 最新更改

[2023-10-29] 新增写日志的功能。

#### 可做的优化

  给用户程序提供`getchar()`等函数。

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





