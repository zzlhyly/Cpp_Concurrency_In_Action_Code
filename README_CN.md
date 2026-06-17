# 《C++并发编程》代码示例

[![C++](https://img.shields.io/badge/C++-11/14-blue.svg)]()
[![VS2017](https://img.shields.io/badge/Build-VS2017-purple.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

[English](README.md) | 中文

《C++ Concurrency in Action》（C++并发编程实战）书籍的代码示例，包含对原书错误代码的修正和改进。

## 目录

- [第6章：基于锁的并发数据结构](#第6章)
- [第7章：无锁并发数据结构](#第7章)
- [第8章：设计并发代码](#第8章)
- [第9章：高级线程管理](#第9章)
- [第10章：测试与调试](#第10章)
- [第11章：消息传递](#第11章)
- [构建说明](#构建说明)
- [贡献指南](#贡献指南)
- [许可证](#许可证)

## 第6章：基于锁的并发数据结构 <a name="第6章"></a>

| 文件 | 说明 |
|------|------|
| `6.1 thread_safe_stack.cpp` | 使用互斥锁实现的线程安全栈 |
| `6.2 thread_safe_queue_with_condition_variable.cpp` | 使用条件变量实现的线程安全队列 |
| `6.3 thread_safe_queue_with_shared_ptr.cpp` | 持有shared_ptr实例的线程安全队列 |
| `6.4 single_thread_queue.cpp` | 单线程版队列实现 |
| `6.5 queue_with_dummy_node.cpp` | 带有虚拟节点的队列 |
| `6.6 thread_safe_queue_fine_grained_locking.cpp` | 细粒度锁版线程安全队列 |
| `6.7 lockable_waitable_thread_safe_queue.cpp` | 可上锁和等待的线程安全队列——内部机构及接口 |
| `6.11 thread_safe_lookup_table.cpp` | 线程安全的查询表 |
| `6.13 thread_safe_list_with_iterator.cpp` | 支持迭代器的线程安全链表 |

## 第7章：无锁并发数据结构 <a name="第7章"></a>

| 文件 | 说明 |
|------|------|
| `7.2.3 lock_free_stack_with_hazard_pointer.cpp` | 使用风险指针实现的无锁栈 |
| `7.9 lock_free_stack_with_lock_free_shared_ptr.cpp` | 使用无锁shared_ptr实现的无锁栈 |
| `7.10 lock_free_stack_with_split_reference_count.cpp` | 使用分离引用计数实现的无锁栈 |
| `7.11 lock_free_stack_split_ref_count_optimized_memory_order.cpp` | 分离引用计数无锁栈（优化内存序） |
| `7.13 lock_free_queue_single_producer_consumer.cpp` | 单生产者-单消费者模型下的无锁队列 |
| `7.15 lock_free_queue_with_reference_count.cpp` | 使用引用计数实现的无锁队列 |
| `7.16 lock_free_queue_ref_count_optimized_push.cpp` | 使用引用计数实现的无锁队列（优化push） |

## 第8章：设计并发代码 <a name="第8章"></a>

| 文件 | 说明 |
|------|------|
| `8.1 parallel_quicksort_with_stack.cpp` | 使用栈的并行快速排序算法 |
| `8.2 parallel_accumulate_original.cpp` | accumulate的原始并行版本（非异常安全） |
| `8.3 parallel_accumulate_with_packaged_task_exception_handling.cpp` | 使用packaged_task的并行accumulate（捕捉异常） |
| `8.3 parallel_accumulate_with_packaged_task.cpp` | 使用packaged_task的并行accumulate（无捕捉异常） |
| `8.4 exception_safe_accumulate.cpp` | 异常安全版accumulate |
| `8.5 exception_safe_parallel_accumulate_with_async.cpp` | 异常安全并行版accumulate——使用async() |
| `8.7 parallel_for_each.cpp` | 并行版for_each |
| `8.8 parallel_for_each_with_async.cpp` | 使用async实现的for_each |
| `8.9 parallel_find.cpp` | 并行find算法实现 |
| `8.10 parallel_find_with_async.cpp` | 使用async实现的并行find算法 |
| `8.11 parallel_partial_sum_with_partitioning.cpp` | 使用划分方式并行计算部分和 |
| `8.13 partial_sum_with_pairwise_update.cpp` | 通过两两更新对的方式实现partial_sum |

## 第9章：高级线程管理 <a name="第9章"></a>

| 文件 | 说明 |
|------|------|
| `9.1 simple_thread_pool.cpp` | 简单的线程池 |
| `9.2 waitable_task_thread_pool.cpp` | 可等待任务的线程池 |
| `9.5 quicksort_with_thread_pool.cpp` | 基于线程池的快速排序实现 |
| `9.6 thread_pool_with_thread_local_queue.cpp` | 线程具有本地任务队列的线程池 |
| `9.8 thread_pool_with_work_stealing.cpp` | 使用任务窃取的线程池 |
| `9.11 interruptible_wait_cv_with_timeout.cpp` | 为condition_variable在interruptible_wait中使用超时 |
| `9.12 interruptible_wait_for_cv_any.cpp` | 为condition_variable_any设计的interruptible_wait |

## 第10章：测试与调试 <a name="第10章"></a>

| 文件 | 说明 |
|------|------|
| `10.1 concurrent_queue_push_pop_test.cpp` | 对队列并发调用push()和pop()的测试用例 |

## 第11章：消息传递 <a name="第11章"></a>

| 文件 | 说明 |
|------|------|
| `11.1 message_passing_framework_atm_example.cpp` | 消息传递框架与完整的ATM示例 |

## 构建说明 <a name="构建说明"></a>

### 前置要求
- Visual Studio 2017 或更高版本
- Windows SDK 10.0 或更高版本

### 构建步骤
1. 克隆仓库
2. 在Visual Studio 2017中打开解决方案文件（`.sln`）
3. 选择配置（Debug/Release）
4. 构建解决方案（`Ctrl+Shift+B`）

### 命令行构建
```batch
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\Current\Bin\MSBuild.exe" /p:Configuration=Release
```

## 贡献指南 <a name="贡献指南"></a>

1. Fork 本仓库
2. 创建特性分支（`git checkout -b feature/amazing-feature`）
3. 提交更改（`git commit -m 'Add amazing feature'`）
4. 推送到分支（`git push origin feature/amazing-feature`）
5. 创建 Pull Request

### 贡献规范
- 遵循现有代码风格和命名规范
- 文件名使用 snake_case 命名
- 保持 chapter.number 前缀格式
- 确保更改可在 VS2017 中编译
- 添加新文件时更新 README

## 许可证 <a name="许可证"></a>

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

## 致谢

- [C++ Concurrency in Action](http://ifeve.com/c-plus-plus-concurrency-in-action/) 作者：Anthony Williams
- 原书代码示例，包含修正和改进