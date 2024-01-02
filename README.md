# GlucOS for UCAS-OS-Lab

### Branch: Project6-File_System

#### 简介

  你说得对，但是 GlucOS 是由仲剑自主研发的一款全新开放的操作系统，程序可以运行在一个被称之为 RISC-V 64 的体系结构上。代号为“西坝河”的第一版本即将发布，已经支持进程调度、同步通信、虚拟内存、设备驱动以及文件系统功能。

  `README-0.md`是做的一些实验记录，主要是给自己看的。

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
NIC=0			# Support NIC driver
GFS_IMG="/home/stu/Ktemp/glucos-img"	# Image file for GFS to protect my SSD
```

  然后，运行`bash ./GlucOS-auto-make.sh clean nodebug`，这将清掉已编译过的文件重新编译，并启用 -O2 优化选项。正常情况下不会有任何错误和警告。

  插入 SD 卡，然后`make floppy`将制作好的镜像写入。启动板卡后`make minicom`，`printlon`，`loadbootm`即可启动 GlucOS。

  补充：如果不使用 PYNQ FPGA 板卡，而是在 QEMU 上运行，请操作：

  (1) 修改`GlucOS-auto-make.sh`脚本中的`GFS_IMG`为一个可用的文件，作为文件系统使用；

  (2) 执行`bash ./GlucOS-auto-make.sh gfs`，这将自动复制镜像到上述位置然后做 40 MiB （如果不够用可修改`Makefile`的`gfs`目标）填充以供文件系统使用；

  (3) 执行`make run-smp ELF_IMAGE=/home/stu/Ktemp/glucos-img`（如果修改了`GFS_IMG`请随之修改）（如果需要网卡驱动功能，请将上面脚本文件中配置`NIC=1`，并且使用`make run-net ELF_IMAGE=/home/stu/Ktemp/glucos-img`），`loadbootm`启动 GlucOS。

#### 运行和测试

##### 1. 文件系统 GFS 初始化

  GlucOS 启动时如果磁盘上没有 GFS 会自动进行初始化。如果存在 GFS 但参数不合法，则会打印出提示，等待用户手动初始化。

  在终端 glush 中执行`mkfs -f`可强制进行 GFS 初始化。

##### 2. GFS 基本操作

  在 glush 中执行`statfs`可查看 GFS 的信息：

```bash
------------------- Terminal -------------------
glucose180@GlucOS:/$ statfs
GFS base sector : 4312
GFS magic       : "Z53/4 Beijingxi-Kunming"
GFS size        : 1024 MiB
inode bitmap loc: 1 Sec
Block bitmap loc: 8 Sec
inode loc       : 72 Sec
Data blocks loc : 3656 Sec
Used inodes     : 3 / 28672 (64 B)
Used data blocks: 4359 / 262144 (4096 B)
GFS cache hit   : R 4145 / 6197, W 0 / 0
glucose180@GlucOS:/$
```

  各行的信息依次为：GFS 在磁盘上的起始扇区号（由系统根据自身大小以及磁盘交换区大小自动计算），GFS 魔数，GFS 总大小，inode 位图起始扇区号（为相对于 GFS 起始扇区号，下同），数据块位图起始扇区号，inode 区起始扇区号，数据块区起始扇区号，已使用的 inode 数及其总数、单个大小，已使用的数据块数及其总数、单个大小，GFS 缓存读写命中情况。

  使用`mkdir`命令可创建目录，使用`cd`命令可切换当前目录。使用`touch`命令可创建文件。以上 3 个命令均应在后面跟路径作为参数。**GFS 支持绝对路径和相对路径，但表达路径时请不要在末尾加多余的斜杠“/”（即，除了表达根目录，否则不要以斜杠结尾），并且路径不要太长（不要超过 70 个字符）。**

```bash
glucose180@GlucOS:/$ mkdir tiepi
glucose180@GlucOS:/$ cd tiepi
glucose180@GlucOS:/tiepi$ touch 12306
glucose180@GlucOS:/tiepi$ cd ..
glucose180@GlucOS:/$
```

  注：“tiepi”即“铁批”~~（iron batch）~~，指对地铁、国铁等轨道交通非常痴迷的人。

  命令提示符“$”前的字符串即为当前目录。使用`ls`命令可查看当前目录下的文件和子目录，加`-l`选项可查看各目录项的信息，加`-lh`选项可以使文件的大小以自动单位来打印。在选项后加目录路径可查看其它目录的情况。

```bash
glucose180@GlucOS:/$ ls
  .   ..   tpm   tp   tp2
  tiepi
glucose180@GlucOS:/$ ls -lh
  0  .            6 Ent
  0  ..           6 Ent
  1  tpm         16 MiB   1 Lnk
1002  tp        1024 KiB   2 Lnk
1002  tp2       1024 KiB   2 Lnk
  2  tiepi        3 Ent
glucose180@GlucOS:/$ ls ./tiepi
  .   ..   12306
glucose180@GlucOS:/$
```

  使用`ls -l`（或`ls -lh`）时，第一列是目录项的 inode 号，第二列是目录项名，第三列是文件大小或目录所含子项数，第四列是文件的硬链接数。

  使用`rm`命令来移除文件。**注意，移除目录时也是使用`rm`命令，且支持递归移除。**

```bash
glucose180@GlucOS:/$ mkdir tiepi/95306 && cd tiepi
glucose180@GlucOS:/tiepi$ ls
  .   ..   12306   95306
glucose180@GlucOS:/tiepi$ cd ..
glucose180@GlucOS:/$ rm tiepi
glucose180@GlucOS:/$
```

##### 3. 文件读写

  最基本的读写测试程序是 rwfile。用法为：`rwfile file`。它将向`file`指定文件中写入 10 行`"Hello world!"`然后读出并打印，也会打印出各系统调用的情况。

  注：由于打印行数比较多，如果觉得字符和终端重叠了可提前用`range`命令调整终端大小，例如`range 13 23`。此外，自 Project 6 开始，glush 支持用省略前缀 “exec” 的方式调用`exec`，例如，启动 rwfile 程序只需要`rwfile`即可。

```bash
sys_open("hello") returns 0
sys_open("hello") returns 0
Hello world!
Hello world!
Hello world!
Hello world!
Hello world!
Hello world!
Hello world!
Hello world!
Hello world!
Hello world!

------------------- Terminal -------------------
glucose180@GlucOS:/$ rwfile hello
glucose180@GlucOS:/$
```

  为了进行大文件的读写测试，可使用增强版的测试程序 rwfile2。其用法为：`rwfile2 [options] file`。支持的选项有：

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

##### 4. 高~~只因~~级文件操作

  为了测试硬链接功能，直接用`ln`命令即可。用法为：`ln file1 file2`，这将为文件`file1`创建硬链接`file2`。例如：

```bash
glucose180@GlucOS:/$ mkdir tiepi
glucose180@GlucOS:/$ ln tp tiepi/tpl
glucose180@GlucOS:/$ ls -lh tiepi
  3  .            3 Ent
  0  ..           5 Ent
  2  tpl       9216 KiB   2 Lnk
glucose180@GlucOS:/$
```

  这为文件`tp`在`tiepi`文件里创建硬链接`tpl`，可见其链接数为 2。

  为了证明硬链接生效，先移除文件`tp`，再对`tiepi/tpl`用 rwfile2 检查数据是否正常，再`ls -lh tiepi`查看其中的硬链接数降为了 1，最后删除目录`tiepi`即可，如下：

```bash
glucose180@GlucOS:/$ rm ./tp
glucose180@GlucOS:/$ rwfile2 -r -m9 tiepi/tpl &
rwfile2: PID is 3
glucose180@GlucOS:/$ ls -lh tiepi
  3  .            3 Ent
  0  ..           4 Ent
  2  tpl       9216 KiB   1 Lnk
glucose180@GlucOS:/$ rm tiepi
glucose180@GlucOS:/$
```

  为了测试用于定位文件指针的`lseek`系统调用，可用测试程序 foptest：`foptest -w -l9437184 -n200 tp`，这将创建文件`tp`，调用`lseek`将文件指针移动到 9437184 字节（9 MiB）处，然后写入 200 个字节的数据（数据内容为字符串`"tiepi"`的不断重复，前 9MiB 自动零填充）。使用`ls -lh`可以看到出现了 9216 KiB 的文件`tp`。

  为了验证写入的数据正确，可用测试程序 cat。其用法为：`cat [options] file`。支持的选项有：

  `-l<loc>`：从第`<loc>`个字节处开始读取；

  `-n<cnt>`：至多读取`<cnt>`个字节；

  `-p<ploc>`：设置屏幕打印区域（纵坐标）为 0 至 `<ploc>`，用于自动滚屏；

  `-x`：使用十六进制显示，而不是字符，用于支持非文本文件。

  在此执行`cat -l9437184 -n200 tp`，可看到屏幕上打印出了连续的“tiepi”字样，代表文件指针定位以及读写成功；执行`cat -l9437100 -n200 -x tp`，可看到前面都是“00”后面出现了非零值，代表零填充正常。打印效果如下：

```bash
...
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 74 69 65 70 69 74 69 65 70 69 74 69
65 70 69 74 69 65 70 69 74 69 65 70 69 74 69 65
...
```

##### 5. 文件系统缓存效果测试

  GlucOS 默认启用文件系统缓存。为了测试效果，可先用`rwfile2 -w -m9 tp`创建一个 9 MiB 的文件`tp`，然后按 Ctrl + A，X 关闭 minicom 再重启开发板及 GlucOS。

  首先执行一次`rwfile2 -r -m9 tp`，这将读取整个 9 MiB 大小的文件`tp`。读取完成后会打印出读取耗时（仅统计读取文件所耗费的时间，不包含检查读出内容的时间）。

  然后再次执行相同命令（可以用`histroy`、`!`命令更方便），比较两次操作显示的耗费的时间。若后一次的时间明显小于前一次，说明文件系统缓存有效。

```bash
glucose180@GlucOS:/$ rwfile2 -r -m9 tp &
rwfile2: PID is 3
glucose180@GlucOS:/$ history
  0  rwfile2 -r -m9 tp &
  1  history
glucose180@GlucOS:/$ !0
  rwfile2 -r -m9 tp &
rwfile2: PID is 3
glucose180@GlucOS:/$
```

第一次：

```bash
9 MiB have been read
Test passed!
Time elapsed: 36485 ms in file RW
```

第二次：

```bash
9 MiB have been read
Test passed!
Time elapsed: 22690 ms in file RW
```

  可以看到第二次的耗时仅为第一次的 $22690/36485\approx62\%$ 左右，说明文件系统缓存成功提高了文件读取操作的性能。

  最后用`statfs`命令查看缓存的命中情况。

```bash
glucose180@GlucOS:/$ statfs        
GFS base sector : 4312      
GFS magic       : "Z53/4 Beijingxi-Kunming"
GFS size        : 1024 MiB                 
inode bitmap loc: 1 Sec   
Block bitmap loc: 8 Sec                    
inode loc       : 72 Sec  
Data blocks loc : 3656 Sec  
Used inodes     : 2 / 28672 (64 B) 
Used data blocks: 2309 / 262144 (4096 B)
GFS cache hit   : R 2341 / 4650, W 0 / 0
```

  两次读取的平均命中率为 $2341/4650\approx 50\%$，这是合理的，因为第一次读取几乎全不命中，第二次读取几乎都命中。

#### 一些思考

  本实验课程就到此结束了，虽然成功打造了一个开源操作系统 GlucOS，但它与真实可用的操作系统相比还存在很多不足。

  (1) 文件系统可靠性：由于文件系统是在持久化存储设备上的，因此它的设计需要有可靠性，但 GlucOS 的文件系统 GFS 可以说根本没有可靠性，如果在执行文件操作过程中发生了宕机或断电，那么 GFS 很可能会出现不一致性或其它异常情况。日志文件系统可以较好地解决可靠性问题，但遗憾的是由于时间和精力等原因这已经不能实现了。

  (2) 文件系统与内核：真实的操作系统中，内核也应该是以文件的形式保存在文件系统中的，但 GlucOS 的内核与 GFS 实际上是分离存放在镜像中的，甚至用户程序也没有保存在文件系统中。要想做到这一点，需要的工作量也是不小的：需要支持初始化文件系统的镜像制作工具，需要支持文件系统的 Bootloader 以及能够解析 ELF 文件的加载器用于启动用户程序……GlucOS 暂无法实现这些。然而，这一缺陷也使得 (1) 似乎不是那么得严重——即使文件系统崩溃，内核竟然也能启动。但是真实的操作系统就需要解决可靠性问题：试想你的 Windows 正在更新，恰好在替换某一系统文件，突然间电源断了，如何保证恢复电源后系统再次启动不会出问题？

  (3) 终端 glush 的命令：在 Linux 系统中，诸如`ls`、`ps`、`kill`等命令实际上都是启动的相应程序，但 glush 把它们都作为内置命令集成了，这使得 glush 变得比较臃肿而且不便维护。在 Project 6 中，GlucOS 首次尝试将原本要用内置命令实现的`cat`做成了独立用户程序并且收获了比较好的效果，但其它的命令仍然以内置命令的形式实现。

  (4) 内核锁的效率：GlucOS 支持 2 个 CPU 核，那就一定要有同步机制来保证对内核临界区的互斥访问。在 Project 3 中 GlucOS 实现了“大内核锁”，或者称为“全局内核锁”（Global Kernel Lock），将整个内核作为临界区用一个自旋锁保护起来。但这样的问题就是两个 CPU 核不能同时进内核（即使它们操作的是内核不相关的两部分）而导致效率低下。使用细粒度内核锁可以提高效率，但这样难度较大且非常容易引入 Bug，故碍于时间问题 GlucOS 没能实现。

  不管怎说，历经 4 个月的艰辛终于把沉重的操作系统研讨课牵引到了终点。感谢大家的支持！希望 GlucOS 的源代码能为今后修读该研讨课的同学提供帮助，如有问题欢迎与作者交流：zhongjian21@mails.ucas.ac.cn。

<img src="images/image-20240102140738340.jpg" alt="image-20240102140738340" style="zoom:80%;" />

（图：HXD3D0141 牵引由昆明站始发的 Z54 次列车进北京西站。拍摄于 2023 年 9 月 2 日。）

