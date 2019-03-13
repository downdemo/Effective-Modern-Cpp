* Modern C++ 一般指 C++11 及其之后的标准，已在工业界被广泛应用。 C++ 初学者适合从 *[C++ Primer](https://learning.oreilly.com/library/view/c-primer-fifth/9780133053043/)* 开始学习 Modern C++ 的基本语法，通过 *[Effective C++](https://learning.oreilly.com/library/view/effective-c-55/0321334876/)* 掌握 C++98 的最佳实践，通过 *[Effective STL](https://learning.oreilly.com/library/view/effective-stl/9780321545183/)* 掌握 STL 的正确使用，通过 *[Effective Modern C++](https://learning.oreilly.com/library/view/effective-modern-c/9781491908419/)* 掌握 C++11/14 的最佳实践，至此即可避开语言缺陷，得心应手地发挥 C++ 的长处。此为个人笔记，还将补充 C++17 相关特性。

## [1. 类型推断](01_deducing_types.html)

* 01 模板类型推断机制
* 02 [auto](https://en.cppreference.com/w/cpp/language/auto) 类型推断机制
* 03 [decltype](https://en.cppreference.com/w/cpp/language/decltype)
* 04 查看推断类型的方法

## [2. auto](02_auto.html)

* 05 用 [auto](https://en.cppreference.com/w/cpp/language/auto) 替代显式类型声明
* 06 [auto](https://en.cppreference.com/w/cpp/language/auto) 推断出非预期类型时，先强制转换出预期类型

## [3. 转向现代 C++](03_moving_to_modern_cpp.html)

* 07 创建对象时注意区分 () 和 {}
* 08 用 [nullptr](https://en.cppreference.com/w/cpp/language/nullptr) 替代 0 和 [NULL](https://en.cppreference.com/w/cpp/types/NULL)
* 09 用 [using 别名声明](https://en.cppreference.com/w/cpp/language/type_alias)替代 [typedef](https://en.cppreference.com/w/cpp/language/typedef)
* 10 用 [enum class](https://en.cppreference.com/w/cpp/language/enum#Scoped_enumerations) 替代 [enum](https://en.cppreference.com/w/cpp/language/enum#Unscoped_enumeration)
* 11 用 =delete 替代 private 作用域来禁用函数
* 12 用 [override](https://en.cppreference.com/w/cpp/language/override) 标记被重写的虚函数
* 13 用 [std::cbegin](https://en.cppreference.com/w/cpp/iterator/begin) 和 [std::cend](https://en.cppreference.com/w/cpp/iterator/end) 获取 const_iterator
* 14 用 [noexcept](https://en.cppreference.com/w/cpp/language/noexcept_spec) 标记不抛异常的函数
* 15 用 [constexpr](https://en.cppreference.com/w/cpp/language/constexpr) 表示编译期常量
* 16 用 [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex) 或 [std::atomic](https://en.cppreference.com/w/cpp/atomic/atomic) 保证 const 成员函数线程安全
* 17 特殊成员函数的隐式合成与抑制机制

## [4. 智能指针](04_smart_pointers.html)

* 18 用 [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr) 管理所有权唯一的资源
* 19 用 [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) 管理所有权可共享的资源
* 20 用 [std::weak_ptr](https://en.cppreference.com/w/cpp/memory/weak_ptr) 观测 [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) 的内部状态
* 21 用 [std::make_unique](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique)（[std::make_shared](https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared)） 创建 [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr)（[std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr)）
* 22 用 [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr) 实现 [pimpl](https://en.cppreference.com/w/cpp/language/pimpl) 必须在源文件中提供析构函数定义

## [5. 右值引用、移动语义和完美转发](05_rvalue_references_move_semantics_and_perfect_forwarding.html)

* 23 [std::move](https://en.cppreference.com/w/cpp/utility/move) 和 [std::forward](https://en.cppreference.com/w/cpp/utility/forward) 只是一种强制类型转换
* 24 转发引用与右值引用的区别
* 25 对右值引用使用 [std::move](https://en.cppreference.com/w/cpp/utility/move)，对转发引用使用 [std::forward](https://en.cppreference.com/w/cpp/utility/forward)
* 26 避免重载使用转发引用的函数
* 27 重载转发引用的替代方案
* 28 引用折叠
* 29 移动不比拷贝快的情况
* 30 无法完美转发的类型

## [6. lambda 表达式](06_lambda_expressions.html)

* 31 捕获的潜在问题
* 32 用初始化捕获将对象移入闭包
* 33 用 [decltype](https://en.cppreference.com/w/cpp/language/decltype) 获取 auto&& 参数类型以 [std::forward](https://en.cppreference.com/w/cpp/utility/forward)
* 34 用 lambda 替代 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind)

## [7. 并发 API](07_the_concurrency_api.html)

* 35 用 [std::async](https://en.cppreference.com/w/cpp/thread/async) 替代 [std::thread](https://en.cppreference.com/w/cpp/thread/thread)
* 36 用 [std::launch::async](https://en.cppreference.com/w/cpp/thread/launch) 指定异步求值
* 37 RAII 线程管理
* 38 [std::future](https://en.cppreference.com/w/cpp/thread/future) 的析构行为
* 39 用 [std::promise](https://en.cppreference.com/w/cpp/thread/promise) 和 [std::future](https://en.cppreference.com/w/cpp/thread/future) 之间的通信实现一次性通知
* 40 [std::atomic](https://en.cppreference.com/w/cpp/atomic/atomic) 提供原子操作，volatile 禁止优化内存

## [8. 其他轻微调整](08_tweaks.html)

* 41 对于可拷贝的形参，如果移动成本低且一定会被拷贝则考虑传值
* 42 用 emplace 操作替代 insert 操作
