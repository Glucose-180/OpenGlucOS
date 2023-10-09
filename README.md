# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 2。

#### 最新更改

[2023-10-09] 系统调用调试成功。

#### 可做的优化

命令提示符格式，原神（`genshin`），启动界面，……

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