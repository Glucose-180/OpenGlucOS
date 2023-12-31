# OS-Lab

这是原神（OS）实验从 Project 2 开始的仓库！

#### 当前项目

Project 4。

#### 最新更改



#### 可做的优化

  在 glush 查询键盘输入时，如果一段时间得不到输入，可以考虑降低查询频率（例如每数秒才查询一次）以降低功耗。

#### [2023-11-13]

  初始化 Project 4 的仓库。

  调整了部分代码的格式；修复了因`allocKernelPage()`等函数失效而导致的`malloc-g.c`编译错误，但该文件中的存储管理函数将在本实验中废除。

  调整了两个 CPU 的内核栈，暂定为`0xffffffc051000000`（0 号 CPU）、`0xffffffc050f00000`。在`head.S`以及`sched.c`中均有定义。

#### [2023-11-13]

  初步写了用虚存启动内核，但存在 Bug 导致内核没有启动。

#### [2023-11-14]

  终于用虚存启动了内核，并且在启动用户程序之前进行了制动（因为尚未实现用户程序的虚存机制）。存在较大的改变：BootLoader 在加载内核时不再一次完成了，而是需要每次 64 个扇区（32 KiB）分步加载。此外，发现一个问题，在`revoke_temp_mapping()`被调用后，即在`boot.c`中对`0x50000000~0x51000000`的临时相等映射被取消后，使用字符串的物理地址调用 BIOS 的`port_write()`会导致错误，修改为内核虚地址后问题解决，推测在 BIOS 中它仍然会使用内核的页表，取消临时映射后就引发了 page fault。此外，用`DEBUG=0`（带 -O2）编译似乎也不会出问题。

#### [2023-11-15]

  为使用虚存启进程做准备，初步填补了`alloc_page_helper()`函数。

  修复了`pg_base`符号冲突问题，并且进行了上板测试，能够用虚存启动内核并制动。

  修改了 createimage 工具以及 GlucOS 中对用户程序的定义`task_info_t`，新增`m_size`成员用于记录该程序在主存中的总大小。

  修改了`create_proc()`、`load_task_img()`、`init_pcb_stack()`等函数使其支持虚存，**但未经过任何测试**；修改了`kmalloc_g()`函数的分配缓冲区为内核 .bss 段的 4 MiB 空间；取消对`umalloc_g()`的支持，但背上了 Project 2 多线程的历史包袱（make 编译时需带上`MTHREAD=0`）；目前，在 QEMU 上内核仍然可以用虚存来启动。

  抛弃了 Project 2 的多线程留下的历史包袱，目前 -O2 上板能启动内核。

  设置`sstatus`的 SUM 为 1，并且增加了注释。

  解除了 CPU 的制动，成功用虚存机制启动用户程序（glush 与 fly）并实现命令行参数传递！-O2 编译下上板正常。但存在严重缺陷，就是给用户程序分配的物理页框以及页表在终止之后不能回收。

  优化了`kmalloc_g()`使用的 4 MiB 的 `kallocbuf`在 .bss 段导致内核启动时清空 .bss 耗时长的问题。

  开始动手修改物理页框管理函数`alloc_page()`，正在准备用类似”位图“的”char map“方法实现物理页框的分配与释放。接下来还要做`free_page()`函数，以及对页表所用页框的类似管理，以及当进程终止时回收其页表和物理页框……目前 -O2 上板正常。

#### [2023-11-17]

  使用“位图”（Char map）方法管理物理页框和页表，但还没有测试过回收`free_page/pagetable()`。目前 -O2 上板正常。下一步：进程终止时回收其页表和页框。

  优化了`SAVE_CONTEXT`与`RESTORE_CONTEXT`，详见`entry.S`中的注释，使之更严谨，以便为后面可能需要的嵌套异常（S 态下的 page fault）做准备。目前 -O2 上板正常。

#### [2023-11-18]

  将 PCB 的`user_stack`改为数组以支持分配多个（通过宏`USTACK_NPG`指定）页面给用户栈，其每个元素是每个物理页框的内核虚地址。新增临时的`handle_pagefault()`，遇到 Page fault 时就终止进程。并且通过了 GlucOStest 的测试。

  实现释放进程页框和页表的函数`free_pages_of_proc()`并且用 GDB 观测正常，-O2 在 QEMU 上运行正常，尚未上板验证。至此 Task 1 算是勉强完成（除了上板）。

  -O2 编译下上板发现有些问题，在启用 GlucOStest 时有概率触发非法指令异常（`0x2`）或取指缺页异常（`0xc`），待修复……

  Bug 复现方法：`exec glush 0 9`，然后`kill 3`，再`exec GlucOStest`。后来发现不用 -O2 也会出错。推测可能跟 TLB 有关系，在终止某个进程之后可能需要刷 TLB。

  进一步细化问题：终止某进程后，再启动别的程序，就会出问题。

  将`do_kill()`中对`free_pages_of_proc()`的调用取消后，就不会出错，推测又触发了 P1-T5 留下的问题：对物理地址`0x52000000`后的某段区域重复写入指令序列会导致错误。

  尝试将`alloc_page()`使用的区域换成物理地址`0x50330000`，仍然不行，看来与位置没有关系。

  终于修好了这个历史遗留问题！原因竟然是……指令 Cache 未同步。

#### [2023-11-20]

  在`do_mbox_recv()`中对传入的地址做了检查，确保其来自用户地址空间，以修复恶意程序传入内核地址导致数据改写的漏洞。新增函数`va2pte()`，根据虚地址和页目录查找页表并返回页表项的内核虚地址，此时`va2kva()`函数改用它实现了。注意，原来的`va2kva()`存在错误，对于 L2 级页表就查到叶节点的情况，返回的地址不正确，忘记了将 PTE 中的物理地址转换为内核虚地址。目前 QEMU 上运行正常。

  初步实现用户栈的缺页处理和按需调页，以及访问地址非法时终止进程。目前 QEMU 上运行正常。下一步：`sys_sbrk()`和数据段的按需调页。

  -O2 上板正常，但发现还有些异常未能添加相应的处理函数，例如非法指令、取指缺页等，一个简单可行的方法是在`handle_other()`中做检查，若未知异常来自用户态则打印一些信息后终止进程。

#### [2023-11-23]

  新增：在`handle_other()`中，当用户程序触发未知异常时，终止进程；`do_sbrk()`函数以及相应的系统调用`sys_sbrk()`。

 使用 gdb 查看 BBL 的指令序列为：

```assembly
(gdb) x/20i 0x50000000
   0x50000000:	csrr	a0,mhartid
   0x50000004:	mv	tp,a0
   0x50000006:	mv	s1,a1
   0x50000008:	auipc	t0,0x58
   0x5000000c:	ld	t0,-1320(t0)
   0x50000010:	csrw	mtvec,t0
   0x50000014:	csrw	mie,zero
  ...
```

  `mtvec`被置为了 `0x50000a30`。

  用 GlucOStest 以及修改的 rw-g 测试了新功能，但在 QEMU 上不能将 U-mode 的异常代理到 S-mode。

  在 make 时用`CFOTHER`可自定义编译选项，加`CFOTHER=-DUSEG_MAX=...`可定义用户程序的 Segment 的允许最大值，将其设为`0x80000000UL`可允许用户使用`sbrk()`分配 1 GiB 以上的空间，但默认是 4 MiB。

  新增数组`pg_uva[]`，用于保存每个物理页框所对的用户虚地址。这样，根据 PID 和虚地址，就能很容易地用`uv2pte()`找到对应的页表项。

  新增文件`mm-swap.c`以及部分相关函数，准备进行与磁盘交换。接下来需要做`swap_from_disk()`，以及在进程结束释放页框的函数`free_pages_of_proc()`中释放其在磁盘中的页。目前 -O2 上板正常。

  新增了完成页交换所需的函数，但未经触发交换的测试。且目前默认新 PTE 的 A、D 均为 1，今后可根据需要在某些情况下不要在一分配时就将 A 置 1。目前 -O2 上板正常，下一步进行交换测试。

  修复了一个小 Bug：用户程序在陷入内核后又触发中断，此时不要对内核上锁，返回时也不要解锁。在 QEMU 上测试通过。但在尝试把`entry.S`中读取`current_running[CPU#]`到`$t0`的代码换成宏时出了错误，等待研究是哪一部分的替换导致的。

#### [2023-11-24]

  最后发现错误在于有个地方要把`current_running[CPU#]`读到`$tp`。现在已经用`GET_CURRENT_RUNNING_TO_T0`替换了所有需要读 c.r. 的地方。

  初步做了页替换，但还存在问题，当有 100 个物理页框和 128 个交换页时，运行`exec swap-test 200`会导致在 191 处出错。

  上个错误是用户程序导致的。修复了异常嵌套中的小问题，现在可以成功处理在陷入内核时的缺页，并且在磁盘 swap 空间不足时会报 panic。

  修复了镜像中 taskinfo 被 swap 空间覆盖的问题，但现在 -O2 上板 swap-test 测 200 个页时会卡死。等待修复……

  修复了上一个错误，是因为在`entry.S`中取 PCB 的`pid`时错误使用了`ld`导致引用了未初始化的内存，进而导致在异常嵌套中重复抢锁。

  页替换初步完成，-O2 上板正常。

  优化了`screen_write()`函数以及相关系统调用，该函数要设置长度，最大由`SCR_WRITE_MAX`指定，默认为 256，但不需要在调用该函数后再刷新屏幕了。系统调用`sys_write()`更名为`sys_screen_write()`，要求传入长度参数，且会对传入的地址做检查。

  修复了测试程序里讨厌的警告。将`pg_uva[]`数组改为`kmalloc_g()`动态分配，以避免清 .bss 段的时间开销。

  swap 功能的测试方法：`make all DEBUG=0 CFOTHER="-DNPF=100"`，限制物理页框数为 100，然后在 glush 中`exec swap-test 200`即可测试交换功能是否正常。

  修改了`Makefile`，开 swap 空间时需要单独`make swap`；修改了`swap-test.c`，使其支持巨量页面（数百 MiB）的测试。

  进行大量测试时出错了，发现访问内存超过数 MiB 就会出奇怪错误。

  修复严重 Bug：`va2pte()`函数中计算的`vpn0`是错误的。目前 -O2 上板正常，正在进行高强度测试：有 200 MiB 左右的内存和 1 GiB 的交换区，用户进程 swap-test 尝试访问约 781 MiB 的地址空间（隔一个页写一个字符，写完再顺序读出检查是否正确）……

  上一个测试在读取时太慢了，可退而求其次：将`NPF`定义为 5120，即 20 MiB 的物理内存，然后`exec swap-test 10240 &`尝试访问 40 MiB，测试通过。截屏保存在“截屏录屏”文件夹。

  准备开多线程，先解决一些历史遗留问题：PCB 中不再保存用户栈页框的内核虚地址，并将用户栈默认大小提高到 16 个页；用户段的最大大小提高到 8 MiB；glush 的初始底端坐标由 24 改为 23 以适应上板的情况；在多个头文件中增设默认宏定义`MULTITHREADING: 1`, `DEBUG_EN: 0`, `TIMER_INTERVAL_MS: 10`；GlucOStest 支持自动滚屏……

  目前 -O2 上板正常。

#### [2023-11-25]

  多线程，启动！初步添加了支持多线程的文件以及创建线程需要的内核函数。目前在 QEMU 上编译正常，运行（似乎）没问题。

  修改了进程调度相关函数以支持多线程，新增多个多线程相关函数。目前 QEMU 上正常。不过，多线程功能至今仍没有测试。

  新增线程相关系统调用，但还未研究 mailbox 并测试。等待测试……

#### [2023-11-26]

  进行了 mailbox 的相关测试，但似乎不太稳定，mailbox 似乎可能会死锁，而且触发过莫名其妙的 S-mode page fault。已在`handle_pagefault()`中加入检查代码，如果同一个进程的某`stval`连续多次出现则 panic 并打印出相关信息。

  上板果然触发了这个 panic，似乎是 1 号 CPU 在初始化`init_exception_s()`时没有设置`sstatus`的 SUM 位导致的。目前 -O2 上板似乎没问题。

  进行 swap 和多线程（甚至带了绑核）联合测试，-O2 上板正常。

  新增编译脚本用于设置 GlucOS 的配置。将 XV6 搬用的丹大师写的存储分配程序`malloc()`等搬到 GlucOS 中，使得`pthread_create()`在给新线程分配栈时不必直接调用`sys_sbrk()`。目前 -O2 上板正常，且进行了用 3 个物理页框（定义`NPF=3`）启 glush 和 fly 的测试，非常卡，每秒换页数十次，但能运行。

  将两年前的线性代数矩阵运算程序 CH3COOC2H5 包装了进去，`LA-MAT.c`作为库函数了。

  glush 新增 history 功能，可输入`history`查看历史命令，以及用`!`加数字的方式执行历史命令。

  修复了 glush 的 history 的小错误。

  可能存在的小 bug：在加载用户进程时，如果物理页框不足发生了页交换，可能会触发`swap_to_disk()`函数刚开始的 panic（新生进程还未被加入 PCB 表其页框就要被换出了）。

  修复了前一个小 bug，改变了`create_proc()`创建进程的执行顺序，先把 PCB 加到`ready_queue`和`pcb_table[]`中再分配页框。但是在 glush 变大后 3 个页框起不来 fly 了，至少要 4 个。除此以外，使 CH3COOC2H5 支持命令行参数调整屏幕区域， -O2 上板正常。

#### [2023-11-27]

  共享内存，（准备）启动！

  新增了共享内存数据结构`shm_ctrl[]`以及开启共享内存的函数`shm_page_get()`。

  共享内存能跑了，但是会出错，会产生很多进程，甚至报页框重复释放的 panic

`[t=0028s] **CPU#0(0) Panic: free_page: page 0xffffffc052007000 is already free**`。

  等待调试。

#### [2023-11-28]

  调试进行中，修复了一些 Bug，当前存在问题鉴定为用户程序 consensus 在调用`is_first()`时，没有把`MAGIC`写到共享内存去，而是似乎把它写到了私有的一个页框了。正在找原因。

  修复成功了，并不是写到别的页框了，而是错误地调用了`clear_pgdir()`导致的。

  仍然存在 Bug，当一个程序使用共享页面且不是第一个时，如果其用户虚地址的 VPN1 在 L1 页表中没有已分配的 PTE，就会导致在`shm_page_get()`中调用`va2pte()`返回的`lpte`不为零从而 panic。程序`shm-test.c`可以起到这个测试效果。

  修复方法：曲线救国，这种情况下先调用`alloc_page_helper()`再将分配的页框释放，使得对应的 PTE 有效。

  连续 3 次执行`exec rw-g 0x12306`会误触发`handle_pagefault()`里的 panic，为此增加时间限制，在 3 秒内同一 PID 的`stval`连续出现 3 次才报 panic。

  在 -O2 条件下，上板进行页交换、多线程、共享内存联合测试，单双核都成功！注：在`USEG_MAX`为 8 MiB 条件下，执行`exec shm-test 8 0x7ff000 &`可以恰好触发 Segment fault，因为这样`sys_sbrk()`能成功但是再申请共享内存就会因为堆过大而得到`NULL`。

  准备进行 fork: Copy-On-Write。规定，被标记为`CMAP_SHARED`（多进程共享）的物理页框对应于数组`pg_uva[]`的值为该页框的引用数（被多少个进程使用）。新增函数`clear_attribute()`用于清 PTE 的某个标志位。目前在 QEMU 上正常。

  fork, 启动！正在进行`do_fork()`代码编写。

#### [2023-11-29]

  原神，继续！初步完成了`do_fork()`、Copy-On-Write 以及页框释放相关代码的编写。尚未经过测试。目前 QEMU 上其它功能正常。下一步：调试 fork。

  fork，启动！初步完成了 fork 和 Copy-On-Write 的测试，测试程序 fork-test-SS 会执行两次`sys_fork()`，一共会出现 4 个进程，并且会使用 barrier、`malloc()`等功能。目前 -O2 上板正常。

  将 fork-test-SS 改为高复杂度测试程序，其行为是：

  (1) 执行`exec fork-test-SS 300 &`启动测试，其中 300 是页交换测试的页框数，若省略则不进行该测试；

  (2) 申请共享内存页并立即解除，以此获得一个新（私有）页，用`p`记录其虚地址，向其中写入字符串“Buying tickets at 95306!”，这是用来测试多线程 Copy-On-Write 的；

  (3) 再次申请共享内存页，用`pshared`记录其地址，向其中写入宏定义的“魔数”`MAGIC`；

  (4) 如果有页交换参数，则启动 swap-test 程序进行换页测试（如果参数足够大，比总页框数还大，则可以将本测试程序的部分页框换出），等待其结束；

  (5) 开始进行 fork，连续两次调用 fork，分出总共 4 个进程，在`test_fork()`函数中调用`malloc()`获取内存，向其中填入不同的字符串并返回其指针，返回后在屏幕不同位置打印“This train is bound for Universal Resort.”等 4 个刚才填入的字符串（中途需要等待数秒），然后通过屏障功能同步；

  (6) 3 个子进程用相同的`SHMKEY`获取共享内存，检查里面的数是否等于`MAGIC`；

  (7) 如果 GlucOS 启用了多线程功能，则创建子线程，让子进程的子线程去向`p`指向的私有页中填入字符串“Deliver goods at 12306!”等，这将触发多线程下的 Copy-On-Write！ 主线程等待子线程退出后，打印出刚才填入的字符串以验证是否正确；

  (8) 等待数秒钟后，打印“Exit!”，所有进程退出，测试结束。

   -O2 上板测试，稳稳的成功！

#### [2023-11-30]

  优化了`panic_g()`，使之成为宏定义，并可自动打印函数名、文件名和行号。参见`glucose.h`。

  学习 XV6 `vm.c`中`walk()`函数的写法优化了`va2pte()`和`alloc_page_helper()`，使之更简洁。但引入了一个问题：`alloc_page_helper()`不再支持对已经分配的页面重新分配，这种情况（根据虚地址查到的 PTE 不为零）将导致 panic！

  学习蒋德钧班实例分析的同学的设计，新增 PTE “奶牛”（COW, Copy-On-Write）属性，参见宏定义`_PAGE_COW`，使用此作为 COW 标志，而非只读位。目前 -O2 上板正常。

  编写 README 文件。

#### [2023-12-01]

  补充 README 文件，准备双仓库开源。

  已进行双仓库开源，在 GitLab 和 GitHub 上各有一个，添加的命令为：

```powershell
PS E:\OneDrive\UCAS-5\操作系统（研讨课）\OpenGlucOS> git remote add public ssh://git@gitlab.agileserve.org.cn:8082/zhongjian21/openglucos.git
PS E:\OneDrive\UCAS-5\操作系统（研讨课）\OpenGlucOS> git push public
...
PS E:\OneDrive\UCAS-5\操作系统（研讨课）\OpenGlucOS> git remote add public2 https://github.com/Glucose-180/GlucOS.git
PS E:\OneDrive\UCAS-5\操作系统（研讨课）\OpenGlucOS> git push public2
...
```

  推送到主仓库（原有的 private 仓库）时，可使用：

```powershell
PS E:\OneDrive\UCAS-5\操作系统（研讨课）\OpenGlucOS> git push origin
Everything up-to-date
```

  GitHub 远端仓库改名为“OpenGlucOS”。注意，使用`git remote set-url public2 https://github.com/Glucose-180/OpenGlucOS.git`，URL 末尾不能加“/”。

  进行了最后的优化（系统启动时直接写入`sstatus`以减少不确定性）和测试。