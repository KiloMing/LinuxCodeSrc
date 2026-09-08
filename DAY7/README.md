# DAY7：pthread 读写锁与单槽生产者—消费者

2026-09-08 学习收尾索引。保留现有 DAY7 结构和练习原稿；多槽 queue/FIFO 仅讨论概念，尚未独立实现。

| 文件 | 内容与状态 |
|---|---|
| [src.cpp](src.cpp) | 2 Reader + 1 Writer 读写锁实验原稿；有未创建 tid4 却 join 的问题，见下方 |
| [rwlock_output_2026-09-08.png](rwlock_output_2026-09-08.png) | 从当天聊天附件补入的原始运行截图，展示多次运行顺序变化 |
| [pdr_cum.cpp](pdr_cum.cpp) | 早期单次单槽同步骨架，只切换 full，未实际写入/消费 data |
| [pdr_cum_final.cpp](pdr_cum_final.cpp) | 循环单槽版本，生产并消费 0～9；原文件名 pdr＿cum_final.cpp 中的全角下划线已改为 ASCII 下划线，内容未改 |
| [thePdrCumMode.png](thePdrCumMode.png) | 单槽流程图原图；图中 no_empty/no_full 对应代码 not_empty/not_full |
| [pdr_cum_final.png](pdr_cum_final.png) | 原有 Linux 运行截图，输出 Consumed data: 0 至 9 |

上述源码和两张生产者—消费者图片已存在于提交 `420f791060ce773ec9a147dbaad0215638b82f5c`；其 Git 时间戳与本次学习日期不同，此处按学习者指定的 2026-09-08 归档，不改写历史日期。rwlock 截图来自引用会话 `6a9f52dd-090c-83ee-87f6-33b2d62d6cd7`。

## 编译和验证

在 DAY7 目录下重新编译，输出放入临时目录，避免覆盖历史二进制：

```sh
g++ -std=c++11 -Wall -Wextra -pthread pdr_cum_final.cpp -o /tmp/day7_producer_consumer
/tmp/day7_producer_consumer
g++ -std=c++11 -Wall -Wextra -pthread src.cpp -o /tmp/day7_rwlock
```

2026-09-08 收尾检查使用当前 macOS 的 clang++：循环单槽代码编译通过（两个未使用参数警告），运行输出与 0～9 的预期逐行一致并正常退出。这是当前主机复核，不等同于重新在 Linux 上验证。原图保存的是学习时 Linux 输出。

rwlock 编译给出未初始化 `tid4` 警告：只创建了 tid1、tid2、tid3，却额外调用 `pthread_join(tid4, nullptr)`。本次保留源码原稿供下次定位修正，没有运行这个含无效 join 的版本，也不把它标记为验证通过。还需关注 pthread 返回值检查。

## 阅读资料时的边界

- `src.cpp` 的 “rdlock” / “write lock” 日志在加锁调用之前，表示即将尝试加锁，不是已经获得锁。并发输出可能交错；创建顺序和某次输出顺序都不是调度保证。
- 流程图展示单次操作；循环版本双方各执行 10 轮。图下方有重复编号/消费框，不能理解成实际消费两次。
- wait 原子地释放 mutex 并等待，返回前重新拿锁；signal 通知等待者，不保证其立即拿到 mutex。被唤醒后仍需 while 重检共享状态。
- DAY7 中已有无扩展名文件是历史 ARM64 Linux 编译产物，本次不删除、不重建，也不作为与当前源码一致的保证。

学习事实与下次测试见 [LINUX-STUDY 当日记录](https://github.com/KiloMing/LINUX-STUDY/blob/main/daily/2026-09-08.md)。
