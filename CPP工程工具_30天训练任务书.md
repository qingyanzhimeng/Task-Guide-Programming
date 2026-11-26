# C++ Linux 工程工具 30 天训练任务书

> 适用背景：已经具备 C++、Linux 系统编程、网络编程、操作系统、数据结构与算法基础；正在从单线程服务器逐步演进到多线程、线程池、select、poll、epoll 与 Reactor。
>
> 建议节奏：每天 1.5～2 小时。时间不足时优先完成“核心任务”，扩展任务可以顺延。
>
> 使用方式：每天完成后把 `- [ ]` 改为 `- [x]`。建议把本文件直接放进你的服务器项目仓库，例如 `docs/30-day-engineering-plan.md`。

---

## 总目标

30 天结束时，你应该能独立完成下面这些事情：

- [ ] 不依赖 AI 自动生成 Makefile / CMakeLists.txt，能够自己维护项目构建系统
- [ ] 能解释 `.cpp → .o → 可执行文件` 的编译、链接过程
- [ ] 能把服务器拆成多个 CMake target，而不是把所有 `.cpp` 塞进一个 executable
- [ ] 能使用 gdb 定位崩溃、调用栈、多线程卡死和异常状态修改
- [ ] 能使用 ASan / UBSan / TSan 检测内存错误、未定义行为和数据竞争
- [ ] 能使用 GoogleTest + CTest 为协议、Buffer、线程池等模块编写自动化测试
- [ ] 能使用 perf 对服务器做基础性能分析
- [ ] 能完成 select / poll / epoll 等服务器版本的统一基准测试
- [ ] 能把 I/O、协议、业务处理、线程池等模块做进一步解耦
- [ ] 能为后续 Reactor 架构建立干净、可测试、可分析的工程骨架

---

# 第一阶段：编译、链接与 CMake
## Day 1～Day 9

---

## Day 1｜理解 C++ 项目是如何变成可执行程序的

### 今日学习知识

- [ ] 理解预处理、编译、汇编、链接四个阶段
- [ ] 理解 `.cpp`、`.h/.hpp`、`.o`、`.a`、`.so`、可执行文件之间的关系
- [ ] 理解“编译错误”和“链接错误”的区别
- [ ] 理解头文件声明与源文件定义的基本关系
- [ ] 理解符号（symbol）在链接过程中的基本作用

### 今日实践任务

- [ ] 手动执行 `g++ -c xxx.cpp -o xxx.o`
- [ ] 手动把多个 `.o` 链接成一个 executable
- [ ] 故意漏掉一个 `.o`，观察 `undefined reference`
- [ ] 故意制造一个语法错误，观察编译错误
- [ ] 使用 `nm` 简单查看一个 `.o` 中的符号
- [ ] 给自己的服务器项目画出当前“源文件 → 目标文件 → 可执行文件”的关系

### 今日产出

- [ ] 写一页笔记解释：compile / link / object file / static library / shared library
- [ ] 能口头回答：为什么修改一个 `.cpp` 文件不应该导致整个工程全部重新编译

### 今日完成

- [ ] Day 1 完成

### 今日记录

> 今天最不清楚的概念：
>
> 今天解决的问题：
>
> 明天需要继续确认：

---

## Day 2｜Makefile 基础：做到能看懂现有工程

### 今日学习知识

- [ ] 理解 Makefile 的 `target`
- [ ] 理解 dependency / prerequisite
- [ ] 理解 recipe
- [ ] 理解变量，如 `CXX`、`CXXFLAGS`
- [ ] 理解 `$@`、`$<`、`$^`
- [ ] 理解增量构建为什么成立
- [ ] 理解时间戳在 Make 构建中的作用

### 今日实践任务

- [ ] 为一个 2～3 个 `.cpp` 的小程序手写 Makefile
- [ ] 修改其中一个源文件，观察只重编译相关目标
- [ ] 执行 `make clean`
- [ ] 阅读你当前服务器项目已有 Makefile
- [ ] 给 Makefile 的每一条规则加注释，说明它做什么

### 今日产出

- [ ] 能独立解释现有 Makefile 的执行流程
- [ ] 明白为什么后面要迁移到 CMake，而不是继续依赖 AI 生成 Makefile

### 今日完成

- [ ] Day 2 完成

### 今日记录

> Makefile 中今天最容易混淆的内容：
>
> 今天第一次真正理解的内容：

---

## Day 3｜CMake 入门：构建第一个 Server / Client

### 今日学习知识

- [ ] 理解 CMake 是“构建系统生成器”，不是编译器
- [ ] 理解 `cmake_minimum_required`
- [ ] 理解 `project`
- [ ] 理解 C++ 标准设置
- [ ] 理解 `add_executable`
- [ ] 理解 source tree 与 build tree 分离
- [ ] 理解 out-of-source build

### 今日实践任务

- [ ] 删除对 AI 生成 Makefile 的依赖
- [ ] 为 server 写第一个 `CMakeLists.txt`
- [ ] 为 client 加入构建
- [ ] 使用 `cmake -S . -B build`
- [ ] 使用 `cmake --build build`
- [ ] 删除 `build/` 后从零重新构建一次

### 今日产出

- [ ] `server` 可以通过 CMake 构建
- [ ] `client` 可以通过 CMake 构建
- [ ] 能解释 `-S` 与 `-B`

### 今日完成

- [ ] Day 3 完成

### 今日记录

> CMake 报错及原因：
>
> 今天学会的 CMake 命令：

---

## Day 4｜现代 CMake：从“文件列表”升级到“Target 思维”

### 今日学习知识

- [ ] 理解 CMake target 是什么
- [ ] 理解 executable target
- [ ] 理解 library target
- [ ] 理解静态库和动态库的基本区别
- [ ] 理解 `add_library`
- [ ] 理解 `target_link_libraries`

### 今日实践任务

把项目拆出至少这些逻辑模块：

- [ ] `protocol`
- [ ] `network`
- [ ] `thread_pool` 或 `concurrency`
- [ ] `server`
- [ ] `client`

并完成：

- [ ] 把协议代码做成 library
- [ ] 把线程池代码做成 library
- [ ] server 链接这些 target
- [ ] client 只链接真正需要的 target
- [ ] 删除明显重复编译的源文件配置

### 今日产出

- [ ] 画出自己的 target 依赖图
- [ ] 能回答：为什么一个工程应该围绕 target 管理，而不是围绕全局 flags 和源文件堆砌

### 今日完成

- [ ] Day 4 完成

---

## Day 5｜include 目录与 PUBLIC / PRIVATE / INTERFACE

### 今日学习知识

- [ ] 理解 `target_include_directories`
- [ ] 理解 `PRIVATE`
- [ ] 理解 `PUBLIC`
- [ ] 理解 `INTERFACE`
- [ ] 理解“依赖传播”
- [ ] 理解 public header 与 private implementation 的区别

### 今日实践任务

- [ ] 规范 `include/` 与 `src/`
- [ ] 给 `protocol` 设置公开 include
- [ ] 给内部实现设置 PRIVATE include
- [ ] 做一个 A → B → C 的最小实验
- [ ] 分别使用 PUBLIC / PRIVATE 看依赖是否传播
- [ ] 修正自己项目里不合理的全局 `include_directories`

### 今日产出

- [ ] 能解释：什么时候用 PUBLIC，什么时候用 PRIVATE
- [ ] 项目不依赖大量全局 include path

### 今日完成

- [ ] Day 5 完成

---

## Day 6｜多目录工程与 add_subdirectory

### 今日学习知识

- [ ] 理解顶层 CMakeLists 与模块 CMakeLists 的职责
- [ ] 理解 `add_subdirectory`
- [ ] 理解模块化构建
- [ ] 理解项目目录与 CMake 依赖结构最好保持一致

### 推荐目录目标

```text
project/
├── CMakeLists.txt
├── include/
│   ├── net/
│   ├── protocol/
│   └── concurrency/
├── src/
│   ├── net/
│   ├── protocol/
│   └── concurrency/
├── apps/
│   ├── server/
│   └── client/
├── tests/
├── benchmark/
└── docs/
```

### 今日实践任务

- [ ] 重构顶层目录
- [ ] 为 `src/net` 建立独立 CMakeLists
- [ ] 为 `src/protocol` 建立独立 CMakeLists
- [ ] 为 `src/concurrency` 建立独立 CMakeLists
- [ ] 为 `apps/server` 与 `apps/client` 建立构建入口
- [ ] 顶层只保留整体工程配置和 `add_subdirectory`

### 今日产出

- [ ] 一个结构清晰的多目录工程
- [ ] 顶层 CMakeLists 不再塞满所有源文件

### 今日完成

- [ ] Day 6 完成

---

## Day 7｜Debug / Release / RelWithDebInfo

### 今日学习知识

- [ ] 理解 Debug
- [ ] 理解 Release
- [ ] 理解 RelWithDebInfo
- [ ] 理解 `-g`
- [ ] 理解优化级别 `-O0/-O2/-O3`
- [ ] 理解为什么性能测试不能直接用 Debug 版本
- [ ] 理解为什么纯 Release 又不方便调试

### 今日实践任务

- [ ] 建立 `build-debug`
- [ ] 建立 `build-release`
- [ ] 比较 executable 大小
- [ ] 对同一段简单代码比较运行时间
- [ ] 打开 `CMAKE_EXPORT_COMPILE_COMMANDS`
- [ ] 查看 `compile_commands.json`

### 今日产出

- [ ] 保留 Debug 与 Release 两套构建目录
- [ ] 能解释未来 gdb / sanitizer / perf 分别更适合什么 build

### 今日完成

- [ ] Day 7 完成

---

## Day 8｜编译警告与编译选项

### 今日学习知识

- [ ] 理解 compiler warning 的价值
- [ ] 理解 `-Wall`
- [ ] 理解 `-Wextra`
- [ ] 理解 `-Wpedantic`
- [ ] 理解 target 级 `target_compile_options`
- [ ] 理解为什么不推荐无脑设置全局 `CMAKE_CXX_FLAGS`

### 今日实践任务

- [ ] 给核心 target 加 warning
- [ ] 修复项目中可以确认的 warning
- [ ] 故意制造未使用变量等 warning
- [ ] 观察编译器信息
- [ ] 确保不通过简单粗暴关闭 warning 解决问题

### 今日产出

- [ ] 项目主要 target 编译基本无 warning
- [ ] 记录至少 3 个 warning 的真实原因

### 今日完成

- [ ] Day 8 完成

---

## Day 9｜CMake 第一阶段验收

### 今日任务

今天不学习新语法。

- [ ] 删除所有 build 目录
- [ ] 从干净项目开始构建 Debug
- [ ] 从干净项目开始构建 Release
- [ ] server 正常生成
- [ ] client 正常生成
- [ ] 所有 library target 正常链接
- [ ] 不让 AI 直接生成 CMakeLists
- [ ] 遇到错误时先自己读错误信息
- [ ] 把当前 CMake 结构画成依赖图
- [ ] 把构建步骤写进 README

### 第一阶段检查点

- [ ] 我能独立添加一个新的 `.cpp/.hpp` 模块
- [ ] 我知道应该把它放到哪个 target
- [ ] 我知道它应该链接谁
- [ ] 我能判断一个错误发生在 compile 还是 link
- [ ] 我能维护当前项目 CMake

### 今日完成

- [ ] Day 9 完成

---

# 第二阶段：GDB 调试
## Day 10～Day 14

---

## Day 10｜GDB 基础控制流程

### 今日学习知识

- [ ] `gdb executable`
- [ ] `run`
- [ ] `break`
- [ ] `next`
- [ ] `step`
- [ ] `continue`
- [ ] `finish`
- [ ] `print`
- [ ] `info locals`

### 今日实践任务

- [ ] Debug 模式编译服务器
- [ ] 在 `main`
- [ ] 在 accept / read 逻辑
- [ ] 在协议解析函数
- [ ] 在任务提交函数分别设置断点
- [ ] 单步跟一次完整请求
- [ ] 观察 fd、buffer size、message type、payload length

### 今日产出

- [ ] 能独立跟踪“一条请求从收到到提交 worker”的执行路径

### 今日完成

- [ ] Day 10 完成

---

## Day 11｜调用栈与崩溃分析

### 今日学习知识

- [ ] `bt`
- [ ] `frame`
- [ ] `up`
- [ ] `down`
- [ ] `info args`
- [ ] `info locals`
- [ ] 理解 stack frame
- [ ] 理解 SIGSEGV 基本含义

### 今日实践任务

- [ ] 故意制造一次空指针访问
- [ ] 故意制造一次无效对象访问
- [ ] 使用 gdb 运行到崩溃
- [ ] 用 `bt` 查看调用栈
- [ ] 切到真正出问题的 frame
- [ ] 查看局部变量
- [ ] 写出崩溃链路

### 今日产出

- [ ] 一份简短崩溃分析记录：现象 → backtrace → 根因 → 修复

### 今日完成

- [ ] Day 11 完成

---

## Day 12｜GDB 多线程调试

### 今日学习知识

- [ ] `info threads`
- [ ] `thread N`
- [ ] `thread apply all bt`
- [ ] 理解 worker thread 在 gdb 中的表现
- [ ] 理解调试多线程程序和单线程程序的主要区别

### 今日实践任务

- [ ] 启动线程池服务器
- [ ] 列出所有线程
- [ ] 找出 main / I/O / worker 对应线程
- [ ] 切换不同线程查看调用栈
- [ ] 在 worker 处理函数中设置断点
- [ ] 观察任务从 queue 被 worker 取走的过程

### 今日产出

- [ ] 画出当前服务器线程模型

### 今日完成

- [ ] Day 12 完成

---

## Day 13｜使用 GDB 分析死锁

### 今日学习知识

- [ ] mutex 死锁复习
- [ ] lock order
- [ ] gdb attach
- [ ] `gdb -p PID`
- [ ] `thread apply all bt`

### 今日实践任务

- [ ] 写一个最小双锁死锁示例
- [ ] 运行至卡死
- [ ] attach 到进程
- [ ] 查看所有线程 backtrace
- [ ] 找到等待关系
- [ ] 根据栈判断锁顺序问题

### 今日产出

- [ ] 一份“如何用 gdb 定位死锁”的笔记

### 今日完成

- [ ] Day 13 完成

---

## Day 14｜Watchpoint 与真实项目调试

### 今日学习知识

- [ ] `watch`
- [ ] `rwatch`
- [ ] `awatch`
- [ ] 理解 watchpoint 与普通 breakpoint 的区别
- [ ] 理解“谁修改了这个状态”类问题

### 今日实践任务

从自己的服务器选一个状态变量：

- [ ] connection state
- [ ] connection count
- [ ] shutdown flag
- [ ] queue size
- [ ] 某个 fd 状态

完成：

- [ ] 设置 watchpoint
- [ ] 触发状态修改
- [ ] 找到修改调用栈
- [ ] 总结此工具适用的问题类型

### GDB 阶段检查点

- [ ] 我会分析 SIGSEGV
- [ ] 我会看多线程调用栈
- [ ] 我会 attach 一个卡死程序
- [ ] 我会判断简单死锁
- [ ] 我会使用 watchpoint 找非法状态修改

### 今日完成

- [ ] Day 14 完成

---

# 第三阶段：Sanitizer
## Day 15～Day 18

---

## Day 15｜AddressSanitizer

### 今日学习知识

- [ ] 理解 ASan 是编译器插桩检测工具
- [ ] heap-buffer-overflow
- [ ] stack-buffer-overflow
- [ ] use-after-free
- [ ] double-free
- [ ] invalid free
- [ ] 基本 leak 现象

### 今日实践任务

写最小程序，分别制造：

- [ ] heap overflow
- [ ] stack overflow
- [ ] use-after-free
- [ ] double-free
- [ ] 越界访问

然后：

- [ ] 使用 ASan 编译
- [ ] 阅读报告
- [ ] 找到出错行
- [ ] 找到 allocation/free stack

### 今日产出

- [ ] 整理 5 种典型内存错误及 ASan 报告特征

### 今日完成

- [ ] Day 15 完成

---

## Day 16｜UBSan + Sanitizer CMake 配置

### 今日学习知识

- [ ] 理解 Undefined Behavior
- [ ] signed integer overflow
- [ ] invalid shift
- [ ] null dereference
- [ ] alignment error
- [ ] sanitizer 编译和链接参数
- [ ] CMake option

### 今日实践任务

- [ ] 给项目增加 `ENABLE_ASAN`
- [ ] 给项目增加 `ENABLE_UBSAN` 或组合配置
- [ ] 建立 `build-asan`
- [ ] 用 ASan/UBSan 构建 server/client
- [ ] 跑一轮真实通信
- [ ] 修复可以确认的问题

### 今日产出

- [ ] sanitizer 不再依赖手写 g++ 命令
- [ ] CMake 可以控制是否启用检测

### 今日完成

- [ ] Day 16 完成

---

## Day 17｜ThreadSanitizer 与数据竞争

### 今日学习知识

- [ ] 什么是 data race
- [ ] race condition 与 data race 的区别
- [ ] mutex / atomic 与 happens-before 基础概念
- [ ] TSan 的基本用途
- [ ] 为什么并发 bug 很难靠普通日志发现

### 今日实践任务

重点检查当前服务器中的：

- [ ] 任务队列
- [ ] 连接容器
- [ ] shutdown flag
- [ ] 统计计数器
- [ ] response queue
- [ ] worker 与 I/O 线程共享的数据

另外：

- [ ] 故意写一个简单 data race
- [ ] 用 TSan 检出
- [ ] 修复后重新运行
- [ ] 再跑自己的服务器

### 今日产出

- [ ] 列出自己项目里“所有跨线程共享对象”

### 今日完成

- [ ] Day 17 完成

---

## Day 18｜Sanitizer 全面验收

### 今日任务

建立并保留：

```text
build-debug/
build-release/
build-asan/
build-tsan/
```

完成：

- [ ] ASan 跑 server/client
- [ ] UBSan 跑 server/client
- [ ] TSan 跑并发 workload
- [ ] 修复至少 1 个自己真实遇到的问题（如果确实没有问题，则记录“未检测到”）
- [ ] 理解为什么 ASan / TSan 不应该简单全部同时开启
- [ ] 把 sanitizer 构建方式记录进 README

### Sanitizer 阶段检查点

- [ ] 我知道内存错误首先应该想到 ASan
- [ ] 我知道数据竞争首先应该想到 TSan
- [ ] 我知道 undefined behavior 可以用 UBSan
- [ ] 我可以通过 CMake 快速切换构建配置

### 今日完成

- [ ] Day 18 完成

---

# 第四阶段：GoogleTest + CTest
## Day 19～Day 23

---

## Day 19｜GoogleTest 基础：先测试应用层协议

### 今日学习知识

- [ ] `TEST`
- [ ] `EXPECT_EQ`
- [ ] `EXPECT_NE`
- [ ] `EXPECT_TRUE`
- [ ] `EXPECT_FALSE`
- [ ] `ASSERT_*` 与 `EXPECT_*` 的区别
- [ ] 单元测试基本思想
- [ ] 测行为而不是测实现细节

### 今日实践任务

对你的应用层协议写：

- [ ] encode/decode 正常测试
- [ ] type 测试
- [ ] length 测试
- [ ] payload 一致性测试
- [ ] 非法输入测试

### 今日产出

- [ ] `tests/protocol_test.cpp`
- [ ] 至少 5 个 protocol test case

### 今日完成

- [ ] Day 19 完成

---

## Day 20｜协议边界测试：半包、粘包、异常包

### 今日学习知识

重点理解：

- [ ] TCP 是字节流
- [ ] TCP 不提供应用消息边界
- [ ] framing
- [ ] fixed header + length field
- [ ] incremental parsing
- [ ] Buffer 的必要性

### 今日测试任务

- [ ] 0 byte payload
- [ ] 最大合法 payload
- [ ] header 不完整
- [ ] payload 不完整
- [ ] 一个 packet 被拆成两次 recv
- [ ] 两个 packet 一次 recv
- [ ] 三个 packet 一次 recv
- [ ] 非法 magic
- [ ] 非法 version
- [ ] 非法 length
- [ ] 恶意超大 length

### 今日产出

- [ ] 一个比“正常收发测试”更可靠的协议测试集合

### 今日完成

- [ ] Day 20 完成

---

## Day 21｜ThreadPool 单元测试

### 今日学习知识

- [ ] 并发组件应该测试什么
- [ ] 测试 completion
- [ ] 测试 shutdown
- [ ] 测试异常路径
- [ ] 避免编写依赖线程执行顺序的脆弱测试

### 今日实践任务

- [ ] 提交 1 个任务
- [ ] 提交 100 个任务
- [ ] 提交 1000 个任务
- [ ] 验证任务全部完成
- [ ] 测试 shutdown
- [ ] 测试重复 shutdown
- [ ] 测试 worker 中任务抛异常时的行为
- [ ] 测试空任务队列状态
- [ ] 如支持返回值，测试 future/result

### 今日产出

- [ ] `tests/thread_pool_test.cpp`

### 今日完成

- [ ] Day 21 完成

---

## Day 22｜CTest 与自动执行测试

### 今日学习知识

- [ ] `enable_testing`
- [ ] GoogleTest 与 CMake 的集成
- [ ] `gtest_discover_tests`
- [ ] `ctest`
- [ ] 测试 target 与生产 target 的依赖关系

### 今日实践任务

- [ ] CMake 构建测试程序
- [ ] `ctest --test-dir build-debug`
- [ ] 所有测试自动执行
- [ ] 测试失败时返回非 0
- [ ] 将 protocol / thread pool / buffer 测试统一接入

### 今日产出

- [ ] 一条命令可以跑完全部单元测试

### 今日完成

- [ ] Day 22 完成

---

## Day 23｜GoogleTest + Sanitizer 组合

### 今日学习知识

- [ ] 为什么单元测试和 sanitizer 组合非常强
- [ ] functional correctness 与 memory/thread correctness 的区别
- [ ] regression test 的意义

### 今日实践任务

- [ ] Debug build 跑 CTest
- [ ] ASan build 跑 CTest
- [ ] UBSan build 跑 CTest
- [ ] TSan build 跑适合的并发测试
- [ ] 修复测试暴露的问题
- [ ] 修改一次核心代码后重新跑完整测试

### 测试阶段检查点

- [ ] 修改协议代码后我有自动回归测试
- [ ] 修改线程池代码后我有自动回归测试
- [ ] 测试不是依赖手工启动多个程序
- [ ] CTest 已经成为常规开发步骤

### 今日完成

- [ ] Day 23 完成

---

# 第五阶段：perf 与服务器性能分析
## Day 24～Day 27

---

## Day 24｜perf stat：建立性能基线

### 今日学习知识

- [ ] profiling 与 benchmark 的区别
- [ ] baseline 的意义
- [ ] `perf stat`
- [ ] task-clock
- [ ] cycles
- [ ] instructions
- [ ] IPC 的基本意义
- [ ] branches
- [ ] branch-misses
- [ ] cache-misses

### 今日实践任务

统一一个 workload，例如：

```text
固定连接数
固定消息数
固定 payload 大小
固定机器
固定 build 类型
```

然后：

- [ ] 使用 Release / RelWithDebInfo
- [ ] 运行 `perf stat`
- [ ] 记录吞吐量
- [ ] 记录 CPU 使用率
- [ ] 记录 cycles
- [ ] 记录 instructions
- [ ] 记录 cache-misses
- [ ] 保存结果

### 今日产出

- [ ] 第一份服务器 performance baseline

### 今日完成

- [ ] Day 24 完成

---

## Day 25｜perf record / perf report：找到 CPU 热点

### 今日学习知识

- [ ] sampling profiler
- [ ] symbol
- [ ] hotspot
- [ ] `perf record`
- [ ] `perf report`
- [ ] 为什么 profiling 需要调试符号
- [ ] 为什么不能只靠“感觉”优化

### 今日实践任务

- [ ] 对服务器执行 `perf record`
- [ ] 使用 `perf report`
- [ ] 找到 Top 10 函数
- [ ] 判断协议解析是否占用明显 CPU
- [ ] 判断 memcpy 是否明显
- [ ] 判断 task queue 是否明显
- [ ] 判断锁竞争相关函数是否明显
- [ ] 记录前三大热点

### 今日产出

- [ ] 一份 hotspot 清单

### 今日完成

- [ ] Day 25 完成

---

## Day 26｜统一 Benchmark：比较服务器架构演进

### 今日学习知识

- [ ] benchmark 的公平性
- [ ] throughput
- [ ] average latency
- [ ] P50 / P95 / P99 latency
- [ ] CPU utilization
- [ ] concurrency
- [ ] workload consistency

### 今日实践任务

如果旧版本代码仍然保留，尽量比较：

- [ ] 单线程 server
- [ ] thread-per-connection
- [ ] thread pool
- [ ] select
- [ ] select + thread pool
- [ ] poll
- [ ] epoll

建立表格：

| Server | Connections | Throughput | Avg Latency | P99 | CPU |
|---|---:|---:|---:|---:|---:|
| Single | | | | | |
| Thread | | | | | |
| Pool | | | | | |
| Select | | | | | |
| Select+Pool | | | | | |
| Poll | | | | | |
| Epoll | | | | | |

### 今日产出

- [ ] 第一版架构性能对比表

### 今日完成

- [ ] Day 26 完成

---

## Day 27｜完成一次“有证据”的性能优化

### 今日原则

今天禁止：

> “我觉得这里可能慢，所以先改。”

必须执行：

```text
Benchmark
→ perf
→ 找热点
→ 提出假设
→ 修改
→ 再 Benchmark
→ 再 perf
```

### 今日实践任务

- [ ] 选一个 perf 明确暴露的热点
- [ ] 写下优化前数据
- [ ] 写下自己的性能假设
- [ ] 修改代码
- [ ] 重跑单元测试
- [ ] 重跑 sanitizer（相关模块）
- [ ] 重跑 benchmark
- [ ] 重跑 perf
- [ ] 比较 before / after
- [ ] 即使没有变快，也记录原因

### 今日产出

写一份：

```text
问题：
证据：
假设：
修改：
结果：
结论：
```

### 今日完成

- [ ] Day 27 完成

---

# 第六阶段：为 Reactor 做工程化准备
## Day 28～Day 30

---

## Day 28｜Reactor 工程骨架设计

### 今日学习知识

重点不是马上抄一个成熟 Reactor，而是理解职责：

- [ ] EventLoop
- [ ] Channel
- [ ] Poller
- [ ] EpollPoller
- [ ] Acceptor
- [ ] TcpConnection
- [ ] TcpServer
- [ ] Buffer
- [ ] Codec
- [ ] ThreadPool / WorkerPool

### 今日设计任务

设计类似目录：

```text
include/net/
├── EventLoop.h
├── Channel.h
├── Poller.h
├── EpollPoller.h
├── Acceptor.h
├── TcpConnection.h
├── TcpServer.h
└── Buffer.h
```

完成：

- [ ] 给每个类写一句职责
- [ ] 给每个类写“它不应该做什么”
- [ ] 画对象关系图
- [ ] 画事件流
- [ ] 规划 target 关系
- [ ] 不照搬某个开源项目的整个设计

### 今日产出

- [ ] `docs/reactor-design.md`

### 今日完成

- [ ] Day 28 完成

---

## Day 29｜I/O 与业务线程彻底解耦

### 今日学习知识

理解推荐的职责边界：

```text
I/O Thread
├── accept
├── read
├── write
├── fd lifecycle
├── epoll_ctl
├── connection state
└── event dispatch

Worker
├── business logic
├── CPU task
├── protocol-level handling
└── response generation
```

### 今日重点问题

- [ ] 谁拥有 fd？
- [ ] 谁允许 close(fd)？
- [ ] worker 能不能直接 `epoll_ctl`？
- [ ] worker 返回结果后如何唤醒 I/O thread？
- [ ] connection 已关闭但 worker 任务还没结束怎么办？
- [ ] 如何避免悬空指针 / use-after-free？
- [ ] response queue 由谁管理？
- [ ] shutdown 时如何安全退出？

### 今日实践任务

- [ ] 画 I/O → Worker → I/O 完整数据流
- [ ] 标出所有跨线程共享对象
- [ ] 标出所有生命周期风险
- [ ] 标出所有需要 mutex / atomic / ownership 设计的位置
- [ ] 为后续 eventfd / wakeup mechanism 留出设计位置

### 今日产出

- [ ] 一张并发架构图
- [ ] 一张 connection lifecycle 图

### 今日完成

- [ ] Day 29 完成

---

## Day 30｜30 天总验收

### 构建验收

- [ ] 删除所有 build 目录
- [ ] 从零配置 Debug
- [ ] 从零构建 Debug
- [ ] 从零配置 Release
- [ ] 从零构建 Release
- [ ] server/client 全部成功
- [ ] 不依赖 AI 生成 CMakeLists

### 测试验收

- [ ] `ctest` 全部通过
- [ ] 协议测试存在
- [ ] Buffer 测试存在（如果已有 Buffer）
- [ ] ThreadPool 测试存在
- [ ] 边界测试存在

### 调试验收

- [ ] 我会 gdb 启动程序
- [ ] 我会 breakpoint
- [ ] 我会 bt
- [ ] 我会查看局部变量
- [ ] 我会切线程
- [ ] 我会查看所有线程调用栈
- [ ] 我会 attach 到运行中的进程

### Sanitizer 验收

- [ ] ASan build 可用
- [ ] UBSan build 可用
- [ ] TSan build 可用
- [ ] 至少实际分析过一次真实服务器

### 性能验收

- [ ] 有 perf stat 记录
- [ ] 有 perf report 记录
- [ ] 有服务器版本性能对比
- [ ] 至少做过一次 profiling-driven optimization

### 架构验收

- [ ] I/O 与 worker 职责清晰
- [ ] protocol 模块独立
- [ ] network 模块独立
- [ ] concurrency 模块独立
- [ ] Reactor 有设计文档
- [ ] 连接生命周期有明确 owner

### 最终完成

- [ ] Day 30 完成
- [ ] 30 天工程工具训练完成 🎉

---

# 建议最终项目结构

```text
CppServer/
├── CMakeLists.txt
├── cmake/
├── include/
│   ├── net/
│   ├── protocol/
│   └── concurrency/
├── src/
│   ├── net/
│   ├── protocol/
│   └── concurrency/
├── apps/
│   ├── server/
│   └── client/
├── tests/
│   ├── protocol_test.cpp
│   ├── buffer_test.cpp
│   └── thread_pool_test.cpp
├── benchmark/
├── docs/
│   ├── protocol.md
│   ├── reactor-design.md
│   ├── architecture.md
│   └── performance.md
└── README.md
```

---

# 30 天成果清单

结束后检查自己是否真正获得了以下成果。

## 构建系统

- [ ] 自己维护 CMake
- [ ] 多 target
- [ ] 多目录
- [ ] Debug / Release
- [ ] Sanitizer build

## 调试能力

- [ ] GDB 崩溃分析
- [ ] 多线程调试
- [ ] 死锁分析
- [ ] Watchpoint

## 正确性

- [ ] GoogleTest
- [ ] CTest
- [ ] ASan
- [ ] UBSan
- [ ] TSan

## 性能

- [ ] Benchmark
- [ ] perf stat
- [ ] perf record
- [ ] perf report
- [ ] 至少一次真实优化

## 网络工程能力

- [ ] 应用层协议
- [ ] 半包/粘包处理
- [ ] Buffer
- [ ] ThreadPool
- [ ] select
- [ ] poll
- [ ] epoll
- [ ] Reactor 设计

---

# 每周复盘模板

建议 Day 7 / 14 / 21 / 30 各写一次。

## 本周我真正掌握了什么？

- 

## 本周遇到过哪些真实 Bug？

- 

## 哪个 Bug 是通过工具而不是“肉眼看代码”找到的？

- 

## 当前项目最混乱的地方是什么？

- 

## 哪个模块最需要重构？

- 

## 哪些知识我只是“看懂”，还没有真正用过？

- 

## 下周优先级

1. 
2. 
3. 

---

# 每日开发规则

这 30 天建议始终遵守：

- [ ] 新增模块时，自己维护 CMake
- [ ] 出现崩溃时，优先 gdb / sanitizer，而不是先问 AI
- [ ] 修改协议或线程池后，先跑测试
- [ ] 做性能优化前，先 benchmark / profile
- [ ] AI 可以解释错误、review 设计，但不要让 AI 包办整个构建系统
- [ ] 每次解决一个真实问题，都留下 3～10 行记录
- [ ] 保留旧版本 benchmark 数据，不要只保留最终版本
- [ ] Reactor 不追求一次写完，优先保证模块职责和生命周期清晰

---

# 最终一句话目标

> 从“我会写 C++ Linux 服务器”，进阶到“我能独立构建、测试、调试、分析、优化并持续演进一个 C++ Linux 网络工程”。
