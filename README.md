# Project3-Interactive_OS_and_Process_Management

#### 简介

  Project 3 完成了 Task 1 ~ Task 4，舍弃了高投资、高风险的 Task 5。

#### 使用

##### 编译

  输入下面的编译命令：

```bash
make all DEBUG=0
```

  其中，`DEBUG=0`的意思是关闭代码中自动写日志等用于调试的功能，**同时启用`-O2`编译优化选项**。（注意看，一个 Warning 都没有。）

##### 运行

  输入下面的命令可以启动 GlucOS。

```bash
make run-smp
loadbootm
```

  为了适配不同大小的显示屏，终端的显示范围是可以动态调节的，使用`range`命令，例如，`range 14 24`将把终端在显示屏上的显示范围（支持**自动滚屏**）改为 14~24。使用`clear`命令可以强制清空显示屏。

  测试信号量、屏障、信箱，分别使用下面的命令：

```bash
exec semaphore &
exec barrier &
exec mbox_server & exec mbox_client &
```

  使用`ps`命令可以查看所有用户进程，使用`kill`命令可以终止指定 PID 的进程。此外，支持使用`&&`连接多条命令，它们将依次执行。例如，`exec waitpid && ps`会在启动`waitpid`并等它运行结束后再执行`ps`，若要在`waitpid`启动后立即运行`ps`，请使用`exec waitpid & && ps`。

  测试双核 CPU 请使用`exec multicore &`。测试绑核请依次执行下面的命令：

```bash
taskset 1 affinity &
taskset -p 2 8
```

  它将以`1`为 CPU 掩码来启动`affinity`，然后将 PID 为 8 的进程的掩码设为`2`从而绑定到 1 号 CPU 上。为了验证，可执行`ps`：

```bash
glucose180@GlucOS:~$ ps                                                       
    PID     STATUS    CPUmask    CMD                                           
    02      Running1    0xff    glush                                         
    03      Sleeping    0x1    affinity                                       
    04      Ready       0x1    test_affinity                                  
    05      Ready       0x1    test_affinity                                   
    06      Ready       0x1    test_affinity                                   
    07      Running0    0x1    test_affinity                                   
    08      Ready       0x2    test_affinity
```

  其中，`Running1`、`Running0`分别表示该进程运行在 1、0 号 CPU 上，可以看到 CPU 掩码已经正确设置。

  GlucOS 的设计追求高鲁棒性，可以看到使用`-O2`编译优化选项仍然不会出问题（至少目前没出过问题）。此外，在运行`semaphore`、`barrier`以及`mbox_client` `mbox_client`期间，使用`kill`命令任意终止正在运行的一个进程（别终止 2 号进程 glush 哈，那样就没办法继续使用终端了），用户程序可能会出问题（运行异常），但是内核不会因此崩掉（为了实现这一鲁棒性，我费了不少心思，因为我认为一个合格的操作系统，不能让用户的非法操作把自己的内核搞崩，至于用户程序的异常，那是用户自己的问题导致的，跟我操作系统没有关系）。

#### 其它

`README-0.md`是做的一些实验记录，主要是给自己看的。

最近一段时间人有点被这种高压环境折磨得精神失常了，真来不及做调试记录了呜呜呜……

