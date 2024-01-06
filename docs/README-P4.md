# GlucOS for UCAS-OS-Lab

### Branch: Project4-Virtual_Memory_Management

#### 简介

  你说得对，但是 GlucOS 是由仲剑自主研发的一款全新开放的操作系统，程序可以运行在一个被称之为 RISC-V 64 的体系结构上。GlucOS 在不断成长，目前已经支持进程调度、同步通信以及虚拟内存功能，尚不支持设备驱动和文件系统。

  `README-0.md`是做的一些实验记录，主要是给自己看的。

#### 编译

  **注意**：请先配置`GlucOS-auto-make.sh`中的参数和选项，再进行编译！

  编辑文件`GlucOS-auto-make.sh`。保持`Multithreading=1`，表示启用多线程功能；`NCPU=2`，代表支持两个 CPU（双核 CPU）；`NPF=200`，限制可用物理页框数为 200（为了测试页交换功能）；`NPSWAP=512`，表示有 512 个磁盘交换区页面可用，建议调大一些；设置`DEBUG=1`，代表启用调试模式，以便通过日志查看系统运行信息。将`User_seg_max`设置为`0x80000000`，允许用户程序占用最大约 2 GiB 的内存；保持`UPROC_MAX=32`，代表最大允许 32 个用户进程。

  配置好信息后，执行下面的命令进行 GlucOS 的编译，正常情况下不会有任何 Warning 和 Error：

```bash
$ bash ./GlucOS-auto-make.sh clean
```

  如果出现“bad interpreter”的错误，可以检查该文件的行尾是否是 LF，如果不是请修改。

  注：为了查看日志，请检查工作目录下有没有`glucos-fpga.log`和`glucos-qemu.log`文件，如果没有请创建。

#### 运行

  将 SD 卡插入，`make floppy`把镜像写入，然后插到开发板上并启动，`make minicom`，等待数秒后依次输入`printlon`、`loadbootm`即可启动 GlucOS，此时`tail -f glucos-fpga.log`可查看打印出的日志信息。

  如果不用硬件开发板，也可以直接`make run-smp`，在 QEMU 中模拟运行，然后输入`loadbootm`即可启动。`tail -f glucos-qemu.log`查看日志。但如果要测试页交换功能，在启动 QEMU 前请执行`make swap NPSWAP=512`将镜像文件扩充作为交换区。

  GlucOS 启动后将自动运行 glush（终端程序）。

  首先，在 glush 中执行`exec fly &`，以及`exec lock 0 & exec lock 1 &`，可验证简单的启进程功能。这是 trivial 的。使用`ps`命令查看正在运行的所有用户进程，使用`kill` + PID 命令可终止进程。

  为了验证动态页表和按需调页功能，请不要直接运行程序 rw。GlucOS 有堆（heap）的概念，不允许用户进程任意访问虚地址，因此请运行修改过的 rw-g 测试程序，它会先用系统调用`sys_sbrk()`申请 1 GiB 的空间，然后根据命令行参数的地址进行读写。如果因为系统`User_seg_max`的限制请求失败，则会重新申请 2 MiB 的空间。

```bash
glucose180@GlucOS:~$ exec rw-g 0x40000000 &
```

  这将尝试对`0x40000000`（即 1 GiB 处）地址进行读写，出现“Success!”代表成功。如果换成非法地址，例如`0x100`，或者是`0x80000000`，则会导致 Segment fault，从而进程被终止。在日志中可以看到进程被终止的信息。

```
[t=0293s] glush: exec rw-g 0x80000000 &
[t=0293s] Process "rw-g" (PID: 3) is created
[t=0293s] Thread 0 of proc 3 is to be killed (EXITING)
[t=0293s] Thread 0 of process 3 is terminated
[t=0293s] Process 3 is terminated and 3 page frames are freed
[t=0298s] glush: exec rw-g 0x100 &
[t=0298s] Process "rw-g" (PID: 3) is created
[t=0298s] Thread 0 of proc 3 is to be killed (EXITING)
[t=0298s] Thread 0 of process 3 is terminated
[t=0298s] Process 3 is terminated and 3 page frames are freed
```

  用`clear`命令可以清屏。

  接下来验证页交换（swap）功能。`exec swap-test 500 &`可启动测试程序 swap-test 来尝试对 500 个页框进行读写，由于已经限制可用物理页框数为 200，这势必导致缺页。这个程序会检查读写的内容是否正确，测试过程中如果正确会在屏幕上打印出读写的字符串内容“Genshin”，全部测试完成后打印“Swapping test passed!”后退出。这期间可以从日志中看到大量的页交换信息。

  GlucOS 曾接受过高强度的测试，在`NPF=51200`（200 MiB 的物理内存）和`NPSWAP=262144`（1 GiB的交换区）的条件下，执行`exec swap-test 200000 &`测试 781 MiB 左右的内存读写，成功进行了写入，但在读取时实在太慢了被迫中止。但在`NPF=5120`（20 MiB 的物理内存）的条件下，测试`exec swap-test 10240 &`测试访问 40 MiB 的内存是完全胜任的。

  注：GlucOS 可以支持异常嵌套，在内核态触发缺页也能处理。测试程序的源代码`swap-test.c`中有一段精心设计的程序：

```c
...
/*
 * Try to cause page fault after trapped to kernel.
 */
sys_screen_write(p + i * Pgsize, strlen("Genshin"));
...
```

  它将一个很可能已被换出到磁盘的页面的地址直接传给系统调用`sys_screen_write()`，从而试图触发陷入内核后的缺页，以验证该功能是否正确。在日志中看到下面的信息，代表确实触发了内核态的缺页异常：

```
...
[t=0087s] Page fault of proc 3 is caused from S-mode: $stval is 0x204d98, $scause is 0xd
...
```

  然后测试多线程功能，按下面这样执行命令即可：

```bash
glucose180@GlucOS:~$ exec mailbox a & exec mailbox b &         
mailbox: PID is 3                                              
mailbox: PID is 4                                              
glucose180@GlucOS:~$ exec mailbox c &                          
mailbox: PID is 5 
```

  这启动 3 个 mailbox 程序，每个程序会开两个线程，测试系统的 mailbox 收发功能是否正常。不过，这 3 个进程又开多线程似乎可能会导致死锁，然而 GlucOS 对于用户程序的死锁采用鸵鸟算法（“是用户的错”），所以当屏幕上的数字不再跳动时将 3 个进程全部终止然后清屏即可。

```bash
glucose180@GlucOS:~$ kill 3 & kill 4 & kill 5                  
proc 3 is terminated                                           
proc 4 is terminated                                           
proc 5 is terminated                                           
glucose180@GlucOS:~$ clear
```

  注：如果感觉终端屏幕区域太小，可使用`range`命令将其扩大，例如，命令`range 10 23`将 glush 的屏幕区域改变到第 10 行到 第 23 行。

  然后测试共享内存功能，执行`exec consensus &`命令，观察现象即可。

  **最后测试 Copy-On-Write 功能。**GlucOS 提供了测试程序 fork-test-SS 作为**压轴出场**，其行为是：

  (1) 执行`exec fork-test-SS 300 &`启动测试，其中 300 是页交换测试的页框数，若省略则不进行该测试；

  (2) 申请共享内存页并立即解除，以此获得一个新（私有）页，用`p`记录其虚地址，向其中写入字符串“Buying tickets at 95306!”，这是用来测试多线程 Copy-On-Write 的；

  (3) 再次申请共享内存页，用`pshared`记录其地址，向其中写入宏定义的“魔数”`MAGIC`；

  (4) 如果有页交换参数，则启动 swap-test 程序进行换页测试（如果参数足够大，比总页框数还大，则可以将本测试程序的部分页框换出），等待其结束；

  (5) 开始进行 fork，连续两次调用 fork，分出总共 4 个进程，在`test_fork()`函数中调用`malloc()`获取内存，向其中填入不同的字符串并返回其指针，返回后在屏幕不同位置打印“This train is bound for Universal Resort.”等 4 个刚才填入的字符串（中途需要等待数秒），然后通过屏障功能同步；这期间，在日志中可看到触发 fork 和 Copy-on-Write 的信息：

```
...
[t=0744s] Proc 3 forked child process 6 and 7 pages are shared.
[t=0744s] Proc 5(0) 0xf0000ff28 caused page 13 copy-on-write
...
```

  (6) 3 个子进程用相同的`SHMKEY`获取共享内存，检查里面的数是否等于`MAGIC`；

  (7) 如果 GlucOS 启用了多线程功能，则创建子线程，让子进程的子线程去向`p`指向的私有页中填入字符串“Deliver goods at 12306!”等，这将触发多线程下的 Copy-On-Write！ 主线程等待子线程退出后，打印出刚才填入的字符串以验证是否正确；

  (8) 等待数秒钟后，打印“Exit!”，所有进程退出，测试结束。

  这个程序将缺页处理、按需调页、页交换、共享内存、Copy-On-Write 和多线程功能联合测试，它充分表明了 GlucOS 应对复杂情况的鲁棒性以及各个功能的协同性！

  当然，GlucOS 也有缺陷。如果一个进程开了多个线程后再进行 fork，系统会拒绝其请求。如果限制物理页框数、磁盘交换区页数太少而进程使用太多，可能导致系统因资源耗尽而卡死或 panic。

#### 附加

  为了展示 GlucOS 的鲁棒性，请重新编译：`bash ./GlucOS-auto-make.sh clean nodebug`，“nodebug”将使用 -O2 优化选项！但这样的话，GlucOS 就不会打印那么多的日志信息了。记得重新写入 SD 卡，如果上板测试的话。

  GlucOS 提供了测试程序 GlucOStest 来展示其鲁棒性。

  首先，执行`exec GlucOStest`（不要加“&”）启动测试程序（如果与终端发生了重叠，可用`range`命令压低终端的屏幕区域）。你将看到下面的提示：

```
== This program is to test the Operating System ==             
 0 - Try to access kernel memory                               
 1 - Check GPRs after a Syscall                                
 2 - Try to access an unaligned address                        
 3 - Use big stack                                             
 4 - Call sys_sbrk() and access                                
 5 - Command line args                                         
 6 - Call sys_getpid()                                         
 7 - Call sys_exit()                                           
 8 - Illegal instruction                                       
 9 - Access wrong address in kernel                            
```

  该程序提供了 10 个测试功能，按相应数字按键开始测试。具体的行为可参见其源代码。下面仅介绍其中几个：

  (0) 尝试直接访问内核内存，这将导致 Segment fault，进程被终止，但是 GlucOS 依然稳定运行；

  (3) 堆栈溢出，它会先开 15 个页面左右的栈空间并向其中读写，测试成功后再次向下开数个页面，这将导致栈溢出，进程被终止，但是 GlucOS 依然稳定运行；

  (8) 尝试执行非法指令（CSR 指令），这将导致非法指令异常，结果同样是进程被终止（注：如果是在 QEMU 上运行，由于 QEMU 上的 U-Boot 没有把这一异常委托给 S 态，因此 GlucOS 无法处理，而是直接陷入 M 态导致系统崩溃，但这显然不是 GlucOS 的问题）；

(9) 尝试通过系统调用`sys_screen_write()`访问内核，首先传入内核地址，这将导致获得错误返回值 0；随后传入无效的用户地址，导致**在内核态触发缺页**，结果同样是进程被终止而 GlucOS 不受影响。

  注：为了避免来回输同一个命令的麻烦，可以使用`history`命令，找到已执行过的命令前的序号，输入`!`+序号可直接重新执行该命令。

  **这个测试的成功意味着 GlucOS 通过虚存机制成功实现了隔离和保护。**

  GlucOS 提供了测试程序 CH3COOC2H5 来测试用户的`malloc()`存储分配函数以及 GlucOS 系统调用`sys_sbrk()`。执行`exec CH3COOC2H5`可启动，这是一个简单的线性代数运算程序，支持矩阵的加、减、数乘、行列式、求逆等运算，效果如下：

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

#####   本次实验到此结束，更多功能请期待后续实验成果……

