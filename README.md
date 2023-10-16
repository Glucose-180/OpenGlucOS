# Project2-A Simple Kernel-Part1

#### 简介

  截止 2023 年 10 月 16 日，已经完成 Task 3。本文档仅介绍到 Part 1 (Task 2) 的用法。

#### 编译

```bash
make all
```

#### 运行

```bash
make run
loadbootd
```

#### 测试

目前的 GlucOS 支持一个简单的伪命令行，可以输入用`&`连接的多个条目用来启动多个用户程序。Part 1 的检查实验可以输入下面的命令：

```bash
print1 & print2 & lock1 & lock2 & fly
```

改变条目的顺序，可以改变它们在队列（`ready_queue`）中的顺序，从而改变调度的顺序，看到不同的效果。

#### 其它

关于本次实验过程中遇到的一些有趣的 Bug 以及它们的调试记录，放在了`~Record`文件夹。

`README-0.md`是做的一些实验记录，主要是给自己看的。