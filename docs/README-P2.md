# Project2-A Simple Kernel-Part2

#### 简介

  截止 2023 年 10 月 26 日，已经完成 Task 5。

#### 编译

```bash
make all
```

  注：修改了 Makefile，可以支持多种选项。

  默认启用多线程支持（Task 5），如需关闭多线程支持，可以：

```bash
make clean
make all MTHREAD=0
```

  默认关闭用户程序主动放弃执行权（Yield），如需启用 Yield，可以：

```bash
make clean
make all YIELD_EN=1
```

  默认定时器周期（进程切换周期）为 40 ms，如需修改（例如修改为 100 ms），可以：

```bash
make clean
make all TINTERVAL=100
```

  **注意：**换用不同参数时，`make clean`是必要的，不过也可以手动删除编译好的文件重新`make`。

#### 运行

```bash
make run
loadbootd
```

#### 测试

目前的 GlucOS 支持一个简单的伪命令行，可以输入用`&`连接的多个条目用来启动多个用户程序。Part 2 Task 4 的检查实验可以输入下面的命令：

```bash
print1 & print2 & lock1 & lock2 & fly & sleep & timer
```

Task 5 的检查，则可以和 fly 一起运行自己做的测试程序：

```bash
mthread0 & fly
```

注：由于我实现了线程的终止（`thread_kill()`），所以测试程序会在两个子线程各主动退出 20 次后将第一个子线程终止掉，然后在第二个子线程继续运行 20 轮后将它也终止掉，然后它会等待 5 秒，重复上面的过程。

#### 其它

关于本次实验过程中遇到的一些有趣的 Bug 以及它们的调试记录，放在了`~Record`文件夹。

`README-0.md`是做的一些实验记录，主要是给自己看的。