# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 3。

#### 最新更改

[2023-10-28] 维护了`pcb_table[]`。

#### 可做的优化

  给用户程序提供`getchar()`等函数。

#### [2023-10-27]

  初始化 Project 3 的仓库。

  修复了编译错误。注意：本实验要实现的 shell 的源代码搬到了在`test/test_project3`文件夹中，叫`glush.c`，原本的源代码在`test/shell.c`。

  修改了部分代码的缩进为 Tab。

  为 glush（自制 shell 终端）提供必要的库函数（`getchar()`、`getline()`等），并实现命令回显。

  制造出`sys_ps()`、`sys_clear()`并在 glush 中支持。

  初步实现`sys_exec()`，但是函数原型与初始代码不一样，我实现的不需要`argc`这个参数。下面，可以考虑删除内核的假命令行了，而让 glush 自启动即可。

  修复了`sys_exec()`的原型，增加了`argc`参数，具体用法为：若`argc`大于最大允许的数值（`ARGC_MAX`，目前为 8）则报错；若`argc < 0`，则根据`argv[]`自动确定`argc`；若`argv`为`NULL`，则`argc`自动为 0；若`argc`比`argv[]`中实际参数数量大，则以实际数量来设置`argc`；若`argc`比`argv[]`中实际参数数量小，则保持`argc`，多余的参数将被忽略。初步测试成功。

  修复了`do_process_show()`，补充了对互斥锁阻塞队列的扫描。

  将宏定义`TASK_MAXNUM`换为`UTASK_MAX`，表示最大支持的用户程序数（当前为 32）；将宏定义`NUM_MAX_TASK`换为`UPROC_MAX`，表示 GlucOS 最大支持的用户进程数（不含 PID 0），当前定义为 16。

  新增了`pcb-table.c`用于维护一个表`pcb_table[]`，它保存当前所有进程的 PCB 的指针以便查找。**每次新增（创建进程）或删除（终止进程）PCB 时， 需要调用其中的函数来更新这个表。**

  临时修复了`syscall.c`中未实现的函数以避免编译器报警告。