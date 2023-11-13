# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 4。

#### 最新更改

[2023-11-13] 初始化 Project 4 的仓库。

#### 可做的优化



#### [2023-11-13]

  初始化 Project 4 的仓库。

  调整了部分代码的格式；修复了因`allocKernelPage()`等函数失效而导致的`malloc-g.c`编译错误，但该文件中的存储管理函数将在本实验中废除。

  调整了两个 CPU 的内核栈，暂定为`0xffffffc051000000`（0 号 CPU）、`0xffffffc050f00000`。在`head.S`以及`sched.c`中均有定义。

