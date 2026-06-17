# C++ Concurrency in Action - Code Examples

[![C++](https://img.shields.io/badge/C++-11/14-blue.svg)]()
[![VS2017](https://img.shields.io/badge/Build-VS2017-purple.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

English | [中文](README_CN.md)

Code examples from "C++ Concurrency in Action" by Anthony Williams, with bug fixes and improvements to the original book code.

## Table of Contents

- [Chapter 6: Lock-Based Concurrent Data Structures](#chapter-6)
- [Chapter 7: Lock-Free Concurrent Data Structures](#chapter-7)
- [Chapter 8: Designing Concurrent Code](#chapter-8)
- [Chapter 9: Advanced Thread Management](#chapter-9)
- [Chapter 10: Testing & Debugging](#chapter-10)
- [Chapter 11: Message Passing](#chapter-11)
- [Building](#building)
- [Contributing](#contributing)
- [License](#license)

## Chapter 6: Lock-Based Concurrent Data Structures <a name="chapter-6"></a>

| File | Description |
|------|-------------|
| `6.1 thread_safe_stack.cpp` | Thread-safe stack using mutex |
| `6.2 thread_safe_queue_with_condition_variable.cpp` | Thread-safe queue using condition variables |
| `6.3 thread_safe_queue_with_shared_ptr.cpp` | Thread-safe queue holding shared_ptr instances |
| `6.4 single_thread_queue.cpp` | Single-threaded queue implementation |
| `6.5 queue_with_dummy_node.cpp` | Queue with dummy/sentinel nodes |
| `6.6 thread_safe_queue_fine_grained_locking.cpp` | Thread-safe queue with fine-grained locking |
| `6.7 lockable_waitable_thread_safe_queue.cpp` | Lockable and waitable queue internals |
| `6.11 thread_safe_lookup_table.cpp` | Thread-safe lookup table |
| `6.13 thread_safe_list_with_iterator.cpp` | Thread-safe list supporting iterators |

## Chapter 7: Lock-Free Concurrent Data Structures <a name="chapter-7"></a>

| File | Description |
|------|-------------|
| `7.2.3 lock_free_stack_with_hazard_pointer.cpp` | Lock-free stack using hazard pointers |
| `7.9 lock_free_stack_with_lock_free_shared_ptr.cpp` | Lock-free stack with lock-free shared_ptr |
| `7.10 lock_free_stack_with_split_reference_count.cpp` | Lock-free stack with split reference counting |
| `7.11 lock_free_stack_split_ref_count_optimized_memory_order.cpp` | Split reference count stack (optimized memory order) |
| `7.13 lock_free_queue_single_producer_consumer.cpp` | Lock-free SPSC queue |
| `7.15 lock_free_queue_with_reference_count.cpp` | Lock-free queue with reference counting |
| `7.16 lock_free_queue_ref_count_optimized_push.cpp` | Lock-free queue with optimized push |

## Chapter 8: Designing Concurrent Code <a name="chapter-8"></a>

| File | Description |
|------|-------------|
| `8.1 parallel_quicksort_with_stack.cpp` | Parallel quicksort using stack |
| `8.2 parallel_accumulate_original.cpp` | Original parallel accumulate (not exception-safe) |
| `8.3 parallel_accumulate_with_packaged_task_exception_handling.cpp` | Parallel accumulate with exception handling |
| `8.3 parallel_accumulate_with_packaged_task.cpp` | Parallel accumulate without exception handling |
| `8.4 exception_safe_accumulate.cpp` | Exception-safe accumulate |
| `8.5 exception_safe_parallel_accumulate_with_async.cpp` | Exception-safe parallel accumulate using async |
| `8.7 parallel_for_each.cpp` | Parallel for_each implementation |
| `8.8 parallel_for_each_with_async.cpp` | Parallel for_each using async |
| `8.9 parallel_find.cpp` | Parallel find algorithm |
| `8.10 parallel_find_with_async.cpp` | Parallel find using async |
| `8.11 parallel_partial_sum_with_partitioning.cpp` | Parallel partial sum with partitioning |
| `8.13 partial_sum_with_pairwise_update.cpp` | Partial sum with pairwise updates |

## Chapter 9: Advanced Thread Management <a name="chapter-9"></a>

| File | Description |
|------|-------------|
| `9.1 simple_thread_pool.cpp` | Simple thread pool implementation |
| `9.2 waitable_task_thread_pool.cpp` | Thread pool with waitable tasks |
| `9.5 quicksort_with_thread_pool.cpp` | Quicksort using thread pool |
| `9.6 thread_pool_with_thread_local_queue.cpp` | Thread pool with thread-local task queues |
| `9.8 thread_pool_with_work_stealing.cpp` | Thread pool with work stealing |
| `9.11 interruptible_wait_cv_with_timeout.cpp` | Interruptible wait for condition_variable |
| `9.12 interruptible_wait_for_cv_any.cpp` | Interruptible wait for condition_variable_any |

## Chapter 10: Testing & Debugging <a name="chapter-10"></a>

| File | Description |
|------|-------------|
| `10.1 concurrent_queue_push_pop_test.cpp` | Test cases for concurrent queue push/pop |

## Chapter 11: Message Passing <a name="chapter-11"></a>

| File | Description |
|------|-------------|
| `11.1 message_passing_framework_atm_example.cpp` | Complete ATM example with message passing framework |

## Building <a name="building"></a>

### Prerequisites
- Visual Studio 2017 or later
- Windows SDK 10.0 or later

### Build Steps
1. Clone the repository
2. Open the solution file (`.sln`) in Visual Studio 2017
3. Select configuration (Debug/Release)
4. Build Solution (`Ctrl+Shift+B`)

### Command Line Build
```batch
"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\Current\Bin\MSBuild.exe" /p:Configuration=Release
```

## Contributing <a name="contributing"></a>

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Contribution Guidelines
- Follow existing code style and naming conventions
- Use snake_case for file names
- Maintain chapter.number prefix format
- Test your changes compile in VS2017
- Update README if adding new files

## License <a name="license"></a>

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [C++ Concurrency in Action](http://ifeve.com/c-plus-plus-concurrency-in-action/) by Anthony Williams
- Original book code examples with corrections and improvements
