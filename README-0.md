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

  原神，继续！新增目录相关数据结构`dir_entry_t`，继续填补初始化相关的函数（`gfs0.c`），还未完成 mkfs 相关的系统调用。修改了`Makefile`和自动编译脚本，支持在编译脚本中自定义镜像文件的位置以减少对 SSD 的磨损。修复了`bootblock.S`中加载内核的小 bug（当内核所占扇区数恰好为 64 的整数倍可能导致读 0 个扇区的请求）。目前可以编译并且启动。

  新增了glush `mkfs`和`statfs`命令以及相关的系统调用，临时修复了`rwfile.c`中系统调用函数不存在导致的编译错误问题。目前 -O2 上板能够正常进行`mkfs`、`mkfs -f`以及`statfs`命令，但是 -O2 编译时喜提一个内存别名警告，有待解决：

```c
./kernel/fs/gfs0.c: In function 'GFS_init':
./kernel/fs/gfs0.c:103:42: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
  103 |  strcpy((char*)((dir_entry_t*)sector_buf)[0].fname, ".");
      |                                          ^
./kernel/fs/gfs0.c:104:28: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
  104 |  ((dir_entry_t*)sector_buf)[0].ino = rdiidx;
      |                            ^
./kernel/fs/gfs0.c:111:28: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
  111 |  ((dir_entry_t*)sector_buf)[0].ino = ((dir_entry_t*)sector_buf)[1].ino
      |                            ^
```

