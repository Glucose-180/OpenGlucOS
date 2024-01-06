# GlucOS for UCAS-OS-Lab

### Branch: V0-Xibahe

#### 简介

  你说得对，但是 GlucOS 是由仲剑自主研发的一款全新开放的操作系统，程序可以运行在一个被称之为 RISC-V 64 的体系结构上。代号为“西坝河”的第一版本（V0）已于 2024 年 1 月 6 日发布，已经支持进程调度、同步通信、虚拟内存、设备驱动以及文件系统功能。

  `docs`目录中保存了历来几次实验项目的`README.md`（说明文档）和`README-0.md`（实验记录）。

#### 编译和启动

  编辑`GlucOS-auto-make.sh`脚本文件，保持当前默认配置即可：

```bash
Multithreading=1
Timer_interval_ms=10
NCPU=2
#NPF=200		# Number of page frames
NPF=51200
NPSWAP=512		# Number of swap pages
DEBUG=1
User_seg_max="0x800000"	# 8 MiB
#User_seg_max="0x80000000"	# 2 GiB
UPROC_MAX=32	# Number of user processes
NIC=1			# Support NIC driver
GFS_IMG="/home/stu/Ktemp/glucos-img"	# Image file for GFS to protect my SSD
```

  然后，运行`bash ./GlucOS-auto-make.sh clean nodebug`，这将清掉已编译过的文件重新编译，并启用 -O2 优化选项。正常情况下不会有任何错误和警告。

  插入 SD 卡，然后`make floppy`将制作好的镜像写入。启动板卡后`make minicom`，`printlon`，`loadbootm`即可启动 GlucOS。

  补充：如果不使用 PYNQ FPGA 板卡，而是在 QEMU 上运行，请操作：

  (1) 修改`GlucOS-auto-make.sh`脚本中的`GFS_IMG`为一个可用的文件，作为文件系统使用；

  (2) 执行`bash ./GlucOS-auto-make.sh gfs`，这将自动复制镜像到上述位置然后做 40 MiB （如果不够用可修改`Makefile`的`gfs`目标）填充以供文件系统使用；

  (3) 执行`make run-net ELF_IMAGE=/home/stu/Ktemp/glucos-img`（如果修改了`GFS_IMG`请随之修改）（如果不需要网卡驱动功能，请将上面脚本文件中配置`NIC=0`，并且使用`make run-smp ELF_IMAGE=/home/stu/Ktemp/glucos-img`），`loadbootm`启动 GlucOS。

#### 运行和测试

  V0 自带的几个测试程序的用法如下。

##### 1. cat

  cat 用于查看文件内容。其用法为：`cat [options] file`。支持的选项有：

  `-l<loc>`：从第`<loc>`个字节处开始读取；

  `-n<cnt>`：至多读取`<cnt>`个字节；

  `-p<loc>`：设置屏幕打印区域（纵坐标）为 0 至 `<loc>`，用于自动滚屏；

  `-x`：使用十六进制显示，而不是字符，用于支持非文本文件。

##### 2. CH3COOC2H5

  这是一个矩阵运算程序，用于测试用户的`malloc()`存储分配函数以及 GlucOS 系统调用`sys_sbrk()`。执行`CH3COOC2H5`可启动，这是一个简单的线性代数运算程序，支持矩阵的加、减、数乘、行列式、求逆等运算，效果如下：

```octave
CH3COOC2H5:1> A = [1,2,3;4,5,6;9,0,6]                          
A =                                                            
       1       2       3                                       
       4       5       6                                       
       9       0       6                                       
CH3COOC2H5:2> det A                                            
ans =                                                          
    -45                                                        
CH3COOC2H5:3> B = inv A                                        
B =                                                            
       -2/3       4/15       1/15                              
       -2/3       7/15       -2/15                             
       1       -2/5       1/15                                 
CH3COOC2H5:4> I = AB* 
...
```

  上面第 1、2、3、4 条命令分别在进行定义矩阵 A、求 A 的行列式、求 A 的逆矩阵 B、求 A、B 的乘积 I 的操作。输入`exit`命令可退出，回到 glush。相关源代码：`CH3COOC2H5.c`、`LA-MAT.c`、`LA-MAT.h`。

##### 3. fly

  小飞~~坤~~机程序，用于在屏幕上打印小飞机。使用时请加上`&`使其在后台，或者是加上数字命令行参数指定其飞行次数（例如，`fly 4`将飞行 4 次）。

##### 4. rwfile2

  rwfile2 是为了进行大文件的读写设计的增强版的测试程序。其用法为：`rwfile2 [options] file`。支持的选项有：

  `-w`：对`file`指定的文件进行写入垃圾值（默认为`0x5`）；

  `-r`：对`file`指定的文件进行读出并比对是否为`0x5`；

  `-n<size>`：以 KiB 为单位进行读写`<size>`次，即读写`<size>` KiB 大小的数据；

  `-m<size>`：以 MiB 为单位进行读写`<size>`次，即读写`<size>` MiB 大小的数据。

  例如，`rwfile2 -w -m9 tp`将创建文件`tp`并写入 9 MiB 的垃圾值（这个过程比较耗时，请等待），并在屏幕上打印写入情况。写入完后`ls -lh`一下，可以看到多了个文件`tp`，其大小为 9216 KiB。

```bash
glucose180@GlucOS:/$ rwfile2 -w -m9 tp &
rwfile2: PID is 3
glucose180@GlucOS:/$ ls -lh
  0  .            4 Ent
  0  ..           4 Ent
  1  tpm         16 MiB   1 Lnk
  2  tp        9216 KiB   1 Lnk
glucose180@GlucOS:/$
```

  然后执行`rwfile2 -r -m9 tp`，将该文件读出并检查是否正常。当 9 MiB 都读出后可看到“Test passed!”提示，代表读出文件内容也正常。

##### 5. nrecv

  nrecv 用于测试网卡的可靠传输（接收），其用法为：`recv3 [options] &` 。支持的选项有：

  `-l<len>`：一次接收`<len>`个字节的数据包；

  `-h`：使用十六进制格式打印；

  `-c`：使用字符格式打印；

  `-ch`：使用不太好的中文格式打印；

  `-p<loc>`：以`<loc>`为打印区域的坐标下界，用于支持自动滚屏，默认为 9。

  首先可就地取材尝试传输`riscv.lds`文件，**请先查看其大小**，假设为 7105 字节，那么请执行`exec recv3 -l7109 -c &`。注意，**长度参数应为文件大小加上 4**，因为发包程序会在前面附上 4 个字节的长度信息。然后以`-m 5 -f ./riscv.lds -l 10 -s 50`（10 和 50 为丢包率和乱序比例，可适当修改，也可不加）为参数在 Linux 虚拟机中运行发包程序，选择 USB 网卡后即可看到传输情况，在 GlucOS 中的 recv3 将打印文件内容，并最后打印出计算的 Fletcher 校验和。为了验证，可编译运行`tools/fletcher16.c`，将文件名`riscv.lds`作为命令行参数传入，即可自动计算其 Fletcher 校验和进行比对。

  注：如果不想费时间打印接收到的文件内容，可在运行 nrecv 时不加`-c`、`-h`、`-ch`选项的任何一个，这将在接收到数据后只打印最后的校验和。测试完后记得终止 nrecv 进程。接收二进制文件也是可以的，但就是不要再开启打印了，看一下校验和就可以说明正确性了，最大测试过 100 KiB 左右的二进制文件（没错，发送的正是发包程序本身）。

##### 6. frecv

  frecv 也是用于网卡接收的，只不过其不在屏幕上打印而是直接输出到文件。用法为：`frecv -l<len> file`。其中，`<len>`是数据包总长度，即文件长度加 4，将`<len> - 4`个字节的有效数据输出到文件`file`。

##### 7. glush

  这是 GlucOS 的终端，系统启动时自动运行。

#### 一些思考

  本实验课程就到此结束了，虽然成功打造了一个开源操作系统 GlucOS，但它与真实可用的操作系统相比还存在很多不足。

  (1) 文件系统可靠性：由于文件系统是在持久化存储设备上的，因此它的设计需要有可靠性，但 GlucOS 的文件系统 GFS 可以说根本没有可靠性，如果在执行文件操作过程中发生了宕机或断电，那么 GFS 很可能会出现不一致性或其它异常情况。日志文件系统可以较好地解决可靠性问题，但遗憾的是由于时间和精力等原因这已经不能实现了。

  (2) 文件系统与内核：真实的操作系统中，内核也应该是以文件的形式保存在文件系统中的，但 GlucOS 的内核与 GFS 实际上是分离存放在镜像中的，甚至用户程序也没有保存在文件系统中。要想做到这一点，需要的工作量也是不小的：需要支持初始化文件系统的镜像制作工具，需要支持文件系统的 Bootloader 以及能够解析 ELF 文件的加载器用于启动用户程序……GlucOS 暂无法实现这些。然而，这一缺陷也使得 (1) 似乎不是那么得严重——即使文件系统崩溃，内核竟然也能启动。但是真实的操作系统就需要解决可靠性问题：试想你的 Windows 正在更新，恰好在替换某一系统文件，突然间电源断了，如何保证恢复电源后系统再次启动不会出问题？

  (3) 终端 glush 的命令：在 Linux 系统中，诸如`ls`、`ps`、`kill`等命令实际上都是启动的相应程序，但 glush 把它们都作为内置命令集成了，这使得 glush 变得比较臃肿而且不便维护。在 Project 6 中，GlucOS 首次尝试将原本要用内置命令实现的`cat`做成了独立用户程序并且收获了比较好的效果，但其它的命令仍然以内置命令的形式实现。

  (4) 内核锁的效率：GlucOS 支持 2 个 CPU 核，那就一定要有同步机制来保证对内核临界区的互斥访问。在 Project 3 中 GlucOS 实现了“大内核锁”，或者称为“全局内核锁”（Global Kernel Lock），将整个内核作为临界区用一个自旋锁保护起来。但这样的问题就是两个 CPU 核不能同时进内核（即使它们操作的是内核不相关的两部分）而导致效率低下。使用细粒度内核锁可以提高效率，但这样难度较大且非常容易引入 Bug，故碍于时间问题 GlucOS 没能实现。

  不管怎说，历经 4 个月的艰辛终于把沉重的操作系统研讨课牵引到了终点。感谢大家的支持！希望 GlucOS 的源代码能为今后修读该研讨课的同学提供帮助，如有问题欢迎与作者交流：zhongjian21@mails.ucas.ac.cn。

<img src="images/image-20240102140738340.jpg" alt="image-20240102140738340" style="zoom:80%;" />

（图：HXD3D0141 牵引由昆明站始发的 Z54 次列车进北京西站。拍摄于 2023 年 9 月 2 日。）

