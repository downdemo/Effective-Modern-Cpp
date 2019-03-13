## 41 对于可拷贝的形参，如果移动成本低且一定会被拷贝则考虑传值

* 一些函数的形参本身就是用于拷贝的，考虑性能，对左值实参应该执行拷贝，对右值实参应该执行移动

```cpp
class A {
 public:
  void f(const std::string& s) { v_.push_back(s); }
  void f(std::string&& s) { v_.push_back(std::move(s)); }

 private:
  std::vector<std::string> v_;
};
```

* 为同一个功能写两个函数太过麻烦，因此改用为参数为转发引用的模板

```cpp
class A {
 public:
  template <typename T>
  void f(T&& s) {
    v_.push_back(std::forward<T>(s));
  }

 private:
  std::vector<std::string> v_;
};
```

* 但模板会带来复杂性，一是模板一般要在头文件中实现，它可能在对象代码中产生多个函数，二是如果传入了不正确的实参类型，将出现十分冗长的错误信息，难以调试。所以最好的方法是，针对左值拷贝，针对右值移动，并且在源码和目标代码中只需要处理一个函数，还能避开转发引用，而这种方法就是按值传递

```cpp
class A {
 public:
  void f(std::string s) { v_.push_back(std::move(s)); }

 private:
  std::vector<std::string> v_;
};
```

* C++98 中，按值传递一定是拷贝构造，但在 C++11 中，只在传入左值时拷贝，如果传入右值则移动

```cpp
A a;
std::string s{"hi"};
a.f(s);     // 以传左值的方式调用
a.f("hi");  // 以传右值的方式调用
```

* 对比不同方法的开销，重载和模板的成本是，对左值一次拷贝，对右值一次移动（此外模板可以用转发实参直接构造，可能一次拷贝或移动都不要）。传值一定会对形参有一次拷贝（左值）或移动构造（右值），之后再移动进容器，因此对左值一次拷贝一次移动，对右值两次移动。对比之下，传值只多出了一次移动，虽然成本高一些，但极大避免了麻烦
* 可拷贝的形参才考虑传值，因为 move-only 类型只需要一个处理右值类型的函数

```cpp
class A {
 public:
  void f(std::unique_ptr<std::string>&& p) { p_ = std::move(p); }

 private:
  std::unique_ptr<std::string> p_;
};
```

* 如果使用传值，则同样的调用需要先移动构造形参，多出了一次移动

```cpp
class A {
 public:
  void f(std::unique_ptr<std::string> p) { p_ = std::move(p); }

 private:
  std::unique_ptr<std::string> p_;
};
```

* 只有当移动成本低时，多出的一次移动才值得考虑，因此应该只对一定会被拷贝的形参传值

```cpp
class A {
 public:
  void f(std::string s) {
    if (s.size() <= 15) {
      v_.push_back(std::move(s));  // 不满足条件则不添加，但比传引用多了一次析构
    }
  }

 private:
  std::vector<std::string> v_;
};
```

* 之前的函数通过构造拷贝，如果通过赋值来拷贝，按值传递可能存在其他额外开销，这取决于很多方面，比如传入类型是否使用动态分配内存、使用动态分配内存时赋值运算符的实现、赋值目标和源对象的内存大小、是否使用 SSO

```cpp
#include <string>
#include <utility>

class A {
 public:
  explicit A(std::string s) : s_(std::move(s)) {}
  void f(std::string s) { s_ = std::move(s); }

 private:
  std::string s_;
};

int main() {
  std::string s{"hello"};
  A a(s);

  std::string x{"hi"};
  /*
   * 额外的分配和回收成本，可能远高于 std::string 的移动成本
   * 传引用则不会有此成本，因为现在 a.s 的长度比之前小
   */
  a.f(x);

  std::string y{"hello world"};
  a.f(y);  // a.s 比之前长，传值和传引用都有额外的分配和回收成本，开销区别不大
}
```

## 42 用 emplace 操作替代 insert 操作

* [std::vector::push_back](https://en.cppreference.com/w/cpp/container/vector/push_back) 对左值和右值的重载为

```cpp
template <typename T, class Allocator = allocator<T>>
class vector {
 public:
  void push_back(const T& x);
  void push_back(T&& x);
};
```

* 直接传入字面值时，会创建一个临时对象。使用 [std::vector::emplace_back](https://en.cppreference.com/w/cpp/container/vector/emplace_back) 则可以直接用传入的实参调用元素的构造函数
* 所有 insert 操作都有对应的 emplace 操作

```
push_back => emplace_back     // std::list、std::deque、std::vector
push_front => emplace_front   // std::list、std::deque、std::forward_list
insert_after => emplace_after // std::forward_list
insert => emplace             // 除 std::forward_list、std::array 外的所有容器
insert => try_emplace         // C++17，std:map、std::unordered_map
emplace_hint                  // 所有关联容器
```

* 即使 insert 函数不需要创建临时对象，也可以用 emplace 函数替代，此时两者本质上做的是同样的事。因此 emplace 函数就能做到 insert 函数能做的所有事，有时甚至做得更好

```cpp
std::vector<std::string> v;
std::string s{"hi"};
// 下面两个调用的效果相同
v.push_back(s);
v.emplace_back(s);
```

* emplace 不一定比 insert 快。之前 emplace 添加元素到容器末尾，该位置不存在对象，因此新值会使用构造方式。但如果添加值到已有对象占据的位置，则会采用赋值的方式，于是必须创建一个临时对象作为移动的源对象，此时 emplace 并不会比 insert 高效

```cpp
std::vector<std::string> v{"hhh", "iii"};
v.emplace(v.begin(), "hi");  // 创建一个临时对象后移动赋值
```

* 对于 [std::set](https://en.cppreference.com/w/cpp/container/set) 和 [std::map](https://en.cppreference.com/w/cpp/container/map)，为了检查值是否已存在，emplace 会为新值创建一个 node，以便能与容器中已存在的 node 进行比较。如果值不存在，则将 node 链接到容器中。如果值已存在，emplace 就会中止，node 会被析构，这意味着构造和析构的成本被浪费了，此时 emplace 不如 insert 高效

```cpp
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <tuple>

class A {
 public:
  A(int a, int b, int c) : a_(a), b_(b), c_(c) {}
  bool operator<(const A &rhs) const {
    return std::tie(a_, b_, c_) < std::tie(rhs.a_, rhs.b_, rhs.c_);
  }

 private:
  int a_;
  int b_;
  int c_;
};

constexpr int n = 100;

void set_emplace() {
  std::set<A> set;
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      for (int k = 0; k < n; ++k) {
        set.emplace(i, j, k);
      }
    }
  }
}

void set_insert() {
  std::set<A> set;
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      for (int k = 0; k < n; ++k) {
        set.insert(A(i, j, k));
      }
    }
  }
}

void test(std::function<void()> f) {
  auto start = std::chrono::system_clock::now();
  f();
  auto stop = std::chrono::system_clock::now();
  std::chrono::duration<double, std::milli> time = stop - start;
  std::cout << std::fixed << std::setprecision(2) << time.count() << " ms\n";
}

int main() {
  test(set_insert);
  test(set_emplace);
  test(set_insert);
  test(set_emplace);
  test(set_insert);
  test(set_emplace);
}
```

* 创建临时对象并非总是坏事。假设给一个存储 [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) 的容器添加一个自定义删除器的 [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) 对象

```cpp
std::list<std::shared_ptr<A>> v;

void f(A*);
v.push_back(std::shared_ptr<A>(new A, f));
// 或者如下，意义相同
v.push_back({new A, f});
```

* 如果使用 [emplace_back](https://en.cppreference.com/w/cpp/container/list/emplace_back) 会禁止创建临时对象。但这里临时对象带来的收益远超其成本。考虑如下可能发生的事件序列：
  * 创建一个 [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) 临时对象
  * [push_back](https://en.cppreference.com/w/cpp/container/list/push_back) 以引用方式接受临时对象，分配内存时抛出了内存不足的异常
  * 异常传到 [push_back](https://en.cppreference.com/w/cpp/container/list/push_back) 之外，临时对象被析构，于是删除器被调用，A 被释放
* 即使发生异常，也没有资源泄露。[push_back](https://en.cppreference.com/w/cpp/container/list/push_back) 的调用中，由 new 构造的 A 会在临时对象被析构时释放。如果使用的是 [emplace_back](https://en.cppreference.com/w/cpp/container/list/emplace_back)，new 创建的原始指针被完美转发到 [emplace_back](https://en.cppreference.com/w/cpp/container/list/emplace_back) 分配内存的执行点。如果内存分配失败，抛出内存不足的异常，异常传到 [emplace_back](https://en.cppreference.com/w/cpp/container/list/emplace_back) 外，唯一可以获取堆上对象的原始指针丢失，于是就产生了资源泄漏
* 实际上不应该把 new A 这样的表达式直接传递给函数，应该单独用一条语句来创建 [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) 再将对象作为右值传递给函数

```cpp
std::shared_ptr<A> p(new A, f);
v.push_back(std::move(p));
// emplace_back 的写法相同，此时两者开销区别不大
v.emplace_back(std::move(p));
```

* emplace 函数在调用 [explicit](https://en.cppreference.com/w/cpp/language/explicit) 构造函数时存在一个隐患

```cpp
std::vector<std::regex> v;
v.push_back(nullptr);  // 编译出错
v.emplace_back(nullptr);  // 编译通过，运行时抛出异常，难以发现此问题
```

* 原因在于 [std::regex](https://en.cppreference.com/w/cpp/regex/basic_regex) 接受 `const char*` 参数的构造函数被声明为 [explicit](https://en.cppreference.com/w/cpp/language/explicit)，用 [nullptr](https://en.cppreference.com/w/cpp/language/nullptr) 赋值要求 [nullptr](https://en.cppreference.com/w/cpp/language/nullptr) 到 [std::regex](https://en.cppreference.com/w/cpp/regex/basic_regex) 的隐式转换，因此不能通过编译

```cpp
std::regex r = nullptr;  // 错误
```

* 而 [emplace_back](https://en.cppreference.com/w/cpp/container/vector/emplace_back) 直接传递实参给构造函数，这个行为在编译器看来等同于

```cpp
std::regex r{nullptr};  // 能编译但会引发异常，未定义行为
```
