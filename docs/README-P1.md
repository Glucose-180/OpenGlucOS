# Project1-BootLoader

##### [2023-09-21] 做完了 task 5！

编译：

```bash
make all
```

运行：

```bash
make run
```

​	运行后，可看到如下提示信息：

```
It's a bootloader...
Loading kernel...
Decompressing kernel...
Decompression is finished. Booting...
bss check: t version: 2
============================
      Hello GlucOS!
============================
glucose180@GlucOS:~$
```

表明内核加载、解压成功并且启动，可以接受用户输入的命令。此时，可以输入下面的命令之一来启动用户程序：

```bash
2048
auipc
bss
data
```

​	此外，可以输入下面的用于整活的命令（纯属搞笑，来源于互联网上的梗）：

```bash
genshin
```

按 q 键即可退出。

​	在等待用户输入命令时，CPU 始终处于工作状态，输入下面的命令可使内核退出，CPU 进入低功耗状态（大概叫“wfi”，等待中断）：

```bash
exit
```

​	为了实现命令输入，我用 bios_getchar() 写了个自带缓冲区的 getchar()，并用它写了个 getline() 可读取一行，然后，又写了个 trim() 函数，用于去除命令两端的空白符。最后，用它们一起写了个 getcmd() 函数，可打印（高仿 Linux 的）命令提示符并读取用户输入的命令（因为有 trim() 函数，所以可以在命令两端加空格）。这些函数定义在 libs/glucose.c 里面，声明在 include/os/glucose.h 里面。

​	调试的简单记录放在了 ~Document 文件夹。