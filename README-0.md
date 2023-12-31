# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 6。

#### 最新更改



#### 可做的优化



#### [2023-12-18]

  初始化 Project 6 的仓库。

#### [2023-12-19]

  原神，启动！

  进一步初始化 Project 6 的仓库。初始化了测试程序，并且 GlucOS 内核支持以宏定义`NIC`来控制是否支持网卡驱动，以避免在使用 QEMU 时被迫`make run-net`导致的启动、关闭耗时长的问题。目前 QEMU 上可以运行。

  初步添加了文件系统 GFS (Glucose File System) 的部分定义，正在进行磁盘 IO 操作的包装（`gfs-io.c`）。

  初步封装了 GFS 磁盘 IO 操作的函数。在`gfs0.c`中添加了 GFS 初始化相关的函数`GFS_check()`和`GFS_init()`，但后者尚未完成（还需要设计目录结构后初始化根目录`/`）初步添加了位图分配函数`GFS_alloc_in_bitmap()`以及读取 inode 需要的函数`GFS_read_inode()`。接下来需要设计目录结构以继续进行实验。目前内核可以编译成功，但用户程序`rwfile.c`因为缺少系统调用接口仍然无法编译。注：至今文件系统的所有功能都没有经过过测试。

#### [2023-12-20]

  原神，继续！新增目录相关数据结构`GFS_dentry_t`，继续填补初始化相关的函数（`gfs0.c`），还未完成 mkfs 相关的系统调用。修改了`Makefile`和自动编译脚本，支持在编译脚本中自定义镜像文件的位置以减少对 SSD 的磨损。修复了`bootblock.S`中加载内核的小 bug（当内核所占扇区数恰好为 64 的整数倍可能导致读 0 个扇区的请求）。目前可以编译并且启动。

  新增了glush `mkfs`和`statfs`命令以及相关的系统调用，临时修复了`rwfile.c`中系统调用函数不存在导致的编译错误问题。目前 -O2 上板能够正常进行`mkfs`、`mkfs -f`以及`statfs`命令，但是 -O2 编译时喜提一个内存别名警告，有待解决：

```c
./kernel/fs/gfs0.c: In function 'GFS_init':
./kernel/fs/gfs0.c:103:42: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
  103 |  strcpy((char*)((GFS_dentry_t*)sector_buf)[0].fname, ".");
      |                                          ^
./kernel/fs/gfs0.c:104:28: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
  104 |  ((GFS_dentry_t*)sector_buf)[0].ino = rdiidx;
      |                            ^
./kernel/fs/gfs0.c:111:28: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
  111 |  ((GFS_dentry_t*)sector_buf)[0].ino = ((GFS_dentry_t*)sector_buf)[1].ino
      |                            ^
```



  在 Linux 上做实验，得到了如下结论：

  (1) `rmdir`命令默认只能移除空文件夹，可用`rm -r`来递归移除非空文件夹；

  (2) 对于文件夹，路径字符串最后一个字符是否带`/`没有区别，但对于普通文件则不能带`/`；

  (3) 移除`.`和`..`是不允许的。

  通过`__attribute__((__may_alias__))`尝试解决上一次的编译警告问题，但没有成功，最后更改了`GFS_init()`中`sector_buf`的类型。新增路径解析、目录项添加相关函数（参见`gfs-dir.c`、`gfs-path.c`），准备支持`mkdir`以及`rm`等操作。目前 -O2 上板正常，但新增的功能都没有测试过。

  新增实现`cd`、`pwd`相关命令的系统调用内核函数（`do_changedir()`和`do_getpath()`），完善了`GFS_add_dentry()`，并且新增搜索目录项的轮子函数`unsigned int search_dentry_in_dir_inode()`。但以上功能仍然未经过测试。目前编译大概是没问题。

#### [2023-12-22]

  原……原神，启……动！新增路径压缩（去除`.`和`..`）函数`path_squeeze()`并完成了单独的测试`tools/path_squeeze.c`，准备应用到`do_changedir()`系统调用函数中，在连接路径时做压缩。

#### [2023-12-23]

  新增`do_mkdir()`以及`do_readdir()`系统调用函数以支持`mkdir`、`ls`命令。新增`GFS_panic()`函数用于在 GFS 发生严重错误时报错并提醒用户重置。目前`ls`似乎正常，但`mkdir`和`cd`仍然不行，会出现“blocks read error!”的错误。等待修复。

  初步修复了错误，目前 -O2 上板在低强度（创建的文件夹数不太多）测试情况下正常。下一步：实现删除命令`rm`和相应的系统调用。

  新增文件链表函数（在`gfs-flist.c`中），记录打开的文件和目录，但其中的函数都没有经过测试。目前 QEMU 运行正常。

#### [2023-12-24]

  增加了 GFS 初始化、进线程切换目录以及进线程创建、终止时对文件链表的操作（同一个进程的不同线程具有独立的当前打开目录`cur_ino`）。

  首次实现了文件和目录的递归删除`do_remove()`，但存在 bug （会有数据块未释放），并且没有经过高强度测试。等待修复。

  修复了上一个问题，错在扫描 inode 的直接指针和间接指针时漏掉了一种情况的`break`，如下第 10、11 行：

```c
...
	for (i = 0U; i < INODE_NDPTR; ++i)
    {
        if (pinode->dptr[i] == INODE_INVALID_PTR)
            break;
        GFS_read_block(pinode->dptr[i], debbuf);
        for (j = 0U; j < DENT_PER_BLOCK; ++j)
            if (debbuf[j].ino == DENTRY_INVALID_INO)
                break;
        if (j < DENT_PER_BLOCK)
            break;
    }
...
```

  新封装路径解析函数`path_anal_2()`，用于拆分父目录和子文件名，并找到父目录的 inode 号。例如，`/home/glucose/tiepi`将被拆分为`/home/glucose`和`tiepi`，返回`/home/glucose`的 inode 号。目前 -O2 上板正常，仍然没有高强度测试。

  新增测试程序 mkdir-test，用于进行高强度创建目录的测试，并且修复了一些小问题（`GFS_init()`时考虑正在打开的文件，`do_readdir()`时考虑目标不是目录要报错的情况）。创建两千个子目录再递归删除，测试通过（-O2 上板）。

  修复小问题：当调用`GFS_read/write_inode()`时应先扫描打开文件链表，因为里面有 inode。如果读 inode 时发现链表里有，则直接从链表中读，这样快；如果写时发现链表里有，则应当同步改写链表里的 inode。理论上讲这种命中的情况只能是要写某目录的 inode，而有某个进程恰好位于此目录，因为如果有文件在链表中的话，系统应拒绝进程打开并改写文件的请求。目前 -O2 上板测试成功。

  尝试优化`GFS_count_in_bitmap()`，使用`__builtin_popcountl()`来进行字中`1`的计数，但是失败了，大概是因为有`-fno-builtin`编译选项；修改了 Makefile，新增`floppy0`，可以用来擦除 SD 卡中的 super block，以避免上次的干扰。

#### [2023-12-25]

  初步实现`do_open()`、`do_close()`，并在 glush 中支持`touch`命令。

#### [2023-12-26]

  新增`do_read()`函数实现，编译运行正常，但该功能没有经过测试。

  新增`do_write()`函数实现。新增 rwfile2 用来测试文件的顺序读写。目前在 QEMU 上已通过了超过 8 MiB 文件的**顺序**读写。

  创建新进程时，新进程的当前目录（`.cpath`和`.cur_ino`）被设为与当前进程相同。修改了 glush，使其支持省略`exec`命令。新增测试程序 cat，用于打印文本文件或二进制文件（用法可直接运行`cat`查看）。

  新增`.lseek()`系统调用以及从 Linux 移植来的测试程序 foptest，用于测试`.lseek()`系统调用。目前似乎测试成功，可以用 foptest 向后移动 9437184 B (9 MiB) 并写入`"tiepi"`后用 cat （`-s`选项可从指定位置开始读）读出。接下来：上板验证，然后做硬链接`.ln`系统调用。

  -O2 上板初步没问题了！

#### [2023-12-27]

  新增硬链接系统调用`.hlink`。但是出了问题，需要修改`remove_dentry_in_dir_inode()`函数，根据文件或子目录名而不是 inode 号进行查找移除。

  修改了`printk.c`中的函数，使`printk()`支持空格填充（例如`printk("%6s, %6d", ...)`）；修复了硬链接后删除会出问题的 bug；修改了`.readdir`系统调用，使其支持打印文件的硬链接数（`nlink`），并且打印格式更好看。目前 QEMU 上测试正常，等待进行上板综合测试。

  修改了多个测试程序，相对统一了命令行参数的用法。foptest: 测试 lseek，在文件不同位置开始读写字符串（`"tiepi"`）；rwfile: 将字符串（`"Hello world!"`）写入文件，然后读出；rwfile2: 以 KiB 为单位将大量垃圾值（`5`）写入或读出文件；cat: 从指定位置开始读指定长度的文件，可以读字符串也可以读二进制（`-x`选项，以十六进制格式打印）。

  上板测试通过。

#### [2023-12-30]

  原神（最后一个任务），启动！新增文件系统缓存，将位图和 inodes 全部存于缓存区，读时直接读缓存，写时同时更新缓存和磁盘（写穿透），但尚未做数据块的缓存。计划使用 LRU 算法，当调用`GFS_read/write_block()`的时候进行缓存查找，但务必保持`GFS_read/write_sec()`是直接从磁盘上获取。

  突然发现一阵操作导致存在 900 多个数据块不能释放（bitmap 中没有清零）的 bug，等待修复…………（出现的条件可能是在某个目录下造一个大文件和上千个子目录再递归删除）。

#### [2023-12-31]

  此外，先运行`foptest tp -l9437184 -w -n100`，再运行`foptest tp -l9437184 -n120 &`会卡死（打印出数据后、调用`sys_close()`之前或过程中）。

  新增了大量的`volatile`限定符，并且修复了`do_write()`中在递归调用以实现零填充时没有增加文件的`size`导致重复分配数据块的问题。目前似乎正常了。



