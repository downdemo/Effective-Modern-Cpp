## 31 捕获的潜在问题

* 值捕获只保存捕获时的对象状态

```cpp
#include <cassert>

int main() {
  int x = 1;
  auto f = [x] { return x; };
  auto g = [x]() mutable { return ++x; };
  x = 42;  // 不改变 lambda 中已经捕获的 x
  assert(f() == 1);
  assert(g() == 2);
}
```

* 引用捕获会保持与被捕获对象状态一致

```cpp
#include <cassert>

int main() {
  int x = 1;
  auto f = [&x] { return x; };
  x = 42;
  assert(f() == 42);
}
```

* 引用捕获时，在捕获的局部变量析构后调用 lambda，将出现空悬引用

```cpp
#include <functional>
#include <iostream>

std::function<void()> f() {
  int x = 0;
  return [&]() { std::cout << x; };
}

int main() {
  f()();  // x 已被析构，值未定义
}
```

* 值捕获的问题比较隐蔽

```cpp
#include <iostream>

struct A {
  auto f() {
    // 如果去掉 =，或改为捕获 i 就会出错
    return [=] { std::cout << i; };
  };
  int i = 1;
};

int main() {
  A a;
  a.f()();  // 1
}
```

* 数据成员位于 lambda 作用域外，不能被捕获，= 捕获的是 this 指针，实际效果相当于引用捕获

```cpp
#include <iostream>

struct A {
  auto f() {
    return [this] { std::cout << i; };  // i 被视为 this->i
  };
  int i = 1;
};

int main() {
  A a;
  auto x = a.f();  // 内部的 i 与 a.i 绑定
  a.i = 2;
  x();  // 2
}
```

* 因此值捕获也可以出现空悬引用

```cpp
#include <iostream>
#include <memory>

struct A {
  auto f() {
    return [this] { std::cout << i; };
  };
  int i = 1;
};

auto g() {
  auto p = std::make_unique<A>();
  return p->f();
}  // p 被析构

int main() {
  g()();  // A 对象 p 已被析构，lambda 捕获的 this 失效，i 值未定义
}
```

* this 指针会被析构，可以值捕获 `*this` 对象，这才是真正的值捕获

```cpp
#include <iostream>
#include <memory>

struct A {
  auto f() {
    return [*this] { std::cout << i; };
  };
  int i = 1;
};

// 现在 lambda 捕获的是对象的一个拷贝，不会被原对象的析构影响
auto g() {
  auto p = std::make_unique<A>();
  return p->f();
}

int main() {
  A a;
  auto x = a.f();  // 只保存此刻的 a.i
  a.i = 2;
  x();  // 1
  g()();  // A 对象 p 已被析构，lambda 捕获的 this 失效，i 值未定义
}
```

* 更细致的解决方法是，捕获数据成员的一个拷贝

```cpp
#include <iostream>
#include <memory>

struct A {
  auto f() {
    int j = i;
    return [j] { std::cout << j; };
  };
  int i = 1;
};

auto g() {
  auto p = std::make_unique<A>();
  return p->f();
}

int main() {
  g()();  // 1
}
```

* C++14 中提供了广义 lambda 捕获（generalized lambda-capture），也叫初始化捕获（init-capture），可以直接在捕获列表中拷贝

```cpp
#include <iostream>
#include <memory>

struct A {
  auto f() {
    return [i = i] { std::cout << i; };
  };
  int i = 1;
};

auto g = [p = std::make_unique<A>()] { return p->f(); };

int main() {
  g()();  // 1
}
```

* 值捕获似乎表明闭包是独立的，与闭包外的数据无关，但并非如此，比如 lambda 可以直接使用 static 变量（但无法捕获）

```cpp
#include <cassert>

static int i = 1;

int main() {
  constexpr auto f = [] { return i; };
  assert(f() == i);
}
```

## 32 用初始化捕获将对象移入闭包

* move-only 类型对象不支持拷贝，只能采用引用捕获

```cpp
auto p = std::make_unique<int>(42);
auto f = [&p]() { std::cout << *p; };
```

* 初始化捕获则支持把 move-only 类型对象移动进 lambda 中

```cpp
auto p = std::make_unique<int>(42);
auto f = [p = std::move(p)]() { std::cout << *p; };
assert(p == nullptr);
```

* 还可以直接在捕获列表中初始化 move-only 类型对象

```cpp
auto f = [p = std::make_unique<int>(42)]() { std::cout << *p; };
```

* 如果不使用 lambda，C++11 中可以封装一个类来模拟 lambda 的初始化捕获

```cpp
class A {
 public:
  A(std::unique_ptr<int>&& p) : p_(std::move(p)) {}
  void operator()() const { std::cout << *p_; }

 private:
  std::unique_ptr<int> p_;
};

auto f = A(std::make_unique<int>(42));
```

* 如果要在 C++11 中使用 lambda 并模拟初始化捕获，需要借助 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind)

```cpp
auto f = std::bind([](const std::unique_ptr<int>& p) { std::cout << *p; },
                   std::make_unique<int>(42));
```

* bind 对象（[std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 返回的对象）中包含传递给 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 的所有实参的拷贝，对于每个左值实参，bind 对象中的对应部分由拷贝构造，对于右值实参则是移动构造。上例中第二个实参是右值，采用的是移动构造，这就是把右值移动进 bind 对象的手法

```cpp
std::vector<int> v;  // 要移动到闭包的对象
// C++14：初始化捕获
auto f = [v = std::move(v)] {};
// C++11：模拟初始化捕获
auto g = std::bind([](const std::vector<int>& v) {}, std::move(v));
```

* 默认情况下，lambda 生成的闭包类的 operator() 默认为 const，闭包中的所有成员变量也会为 const，因此上述模拟中使用的 lambda 形参都为 const

```cpp
auto f = [](auto x, auto y) { return x < y; };

// 上述 lambda 相当于生成如下匿名类
struct X {
  template <typename T, typename U>
  auto operator()(T x, U y) const {
    return x < y;
  }
};
```

* 如果是可变 lambda，则闭包中的 operator() 就不会为 const，因此模拟可变 lambda 则模拟中使用的 lambda 形参就不必声明为 const

```cpp
std::vector<int> v;
// C++14：初始化捕获
auto f = [v = std::move(v)]() mutable {};
// C++11：模拟可变 lambda 的初始化捕获
auto g = std::bind([](std::vector<int>& v) {}, std::move(v));
```

* 因为 bind 对象的生命期和闭包相同，所以对 bind 对象中的对象和闭包中的对象可以用同样的手法处理

## 33 用 [decltype](https://en.cppreference.com/w/cpp/language/decltype) 获取 auto&& 参数类型以 [std::forward](https://en.cppreference.com/w/cpp/utility/forward)

* 对于泛型 lambda

```cpp
auto f = [](auto x) { return g(x); };
```

* 同样可以使用完美转发

```cpp
// 传入参数是 auto，类型未知，std::forward 的模板参数应该是什么？
auto f = [](auto&& x) { return g(std::forward < ? > (x)); };
```

* 此时可以用 decltype 判断传入的实参是左值还是右值
  * 如果传递给 auto&& 的实参是左值，则 x 为左值引用类型，decltype(x) 为左值引用类型
  * 如果传递给 auto&& 的实参是右值，则 x 为右值引用类型，decltype(x) 为右值引用类型

```cpp
auto f = [](auto&& x) { return g(std::forward<decltype(x)>(x)); };
```

* 转发任意数量的实参

```cpp
auto f = [](auto&&... args) {
  return g(std::forward<decltype(args)>(args)...);
};
```

## 34 用 lambda 替代 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind)

* lambda 代码更简洁

```cpp
int l = 0;
int r = 9;
auto f = [l, r](const auto& x) { return l <= x && x <= r; };

// 用 std::bind 实现相同效果
using namespace std::placeholders;
// C++14
auto f = std::bind(std::logical_and<>{}, std::bind(std::less_equal<>{}, l, _1),
                   std::bind(std::less_equal<>(), _1, r));
// C++11
auto f = std::bind(std::logical_and<bool>{},
                   std::bind(std::less_equal<int>{}, l, _1),
                   std::bind(std::less_equal<int>{}, _1, r));
```

* lambda 可以指定值捕获和引用捕获，而 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 总会按值拷贝实参，要按引用传递则需要使用 [std::ref](https://en.cppreference.com/w/cpp/utility/functional/ref)

```cpp
void f(const A&);

using namespace std::placeholders;
A a;
auto g = std::bind(f, std::ref(a), _1);
```

* lambda 中可以正常使用重载函数，而 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 无法区分重载版本，为此必须指定对应的函数指针类型。lambda 闭包类的 operator() 采用的是能被编译器内联的常规的函数调用，而 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 采用的是一般不会被内联的函数指针调用，这意味着 lambda 比 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 运行得更快

```cpp
void f(int) {}
void f(double) {}

auto g = [] { f(1); };                                 // OK
auto g = std::bind(f, 1);                              // 错误
auto g = std::bind(static_cast<void (*)(int)>(f), 1);  // OK
```

* 实参绑定的是 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 返回的对象，而非内部的函数

```cpp
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

void f(std::chrono::steady_clock::time_point t, int i) {
  std::this_thread::sleep_until(t);
  std::cout << i;
}

int main() {
  using namespace std::chrono_literals;
  using namespace std::placeholders;

  auto g1 = [](int i) { f(std::chrono::steady_clock::now() + 3s, i); };
  g1(1);  // 3 秒后打印 1

  auto g2 = std::bind(f, std::chrono::steady_clock::now() + 3s, _1);
  g2(1);  // 调用 std::bind 后的 3 秒打印 1，而非调用 f 后的 3 秒
}
```

* 上述代码的问题在于，计算时间的表达式作为实参被传递给 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind)，因此计算发生在调用 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 的时刻，而非调用其绑定的函数的时刻。解决办法是延迟到调用绑定的函数时再计算表达式值，这可以通过在内部再嵌套一个 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 来实现

```cpp
auto g = std::bind(f,
                   std::bind(std::plus<>{}, std::chrono::steady_clock::now(),
                             std::chrono::seconds{3}),
                   std::placeholders::_1);
```

* C++14 中没有需要使用 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 的理由，C++11 由于特性受限存在两个使用场景，一是模拟 C++11 缺少的移动捕获，二是函数对象的 operator() 是模板时，若要将此函数对象作为参数使用，用 [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 绑定才能接受任意类型实参

```cpp
struct X {
  template <typename T>
  void operator()(const T&) const;
};

X x;
auto f = std::bind(x, _1);  // f 可以接受任意参数类型
```

* C++11 的 lambda 无法达成上述效果，但 C++14 可以

```cpp
X a;
auto f = [a](const auto& x) { a(x); };
```

### [std::bind](https://en.cppreference.com/w/cpp/utility/functional/bind) 用法示例

* 占位符

```cpp
#include <functional>
#include <iostream>

void f(int a, int b, int c) { std::cout << a << b << c; }

int main() {
  using namespace std::placeholders;
  auto x = std::bind(f, _2, _1, 3);
  // _n 表示 f 的第 n 个参数
  // x(a, b, c) 相当于 f(b, a, 3);
  x(4, 5, 6);  // 543
}
```

* 传引用

```cpp
#include <cassert>
#include <functional>

void f(int& n) { ++n; }

int main() {
  int n = 1;
  auto x = std::bind(f, n);
  x();
  assert(n == 1);
  auto y = std::bind(f, std::ref(n));
  y();
  assert(n == 2);
}
```

* 传递占位符给其他函数

```cpp
#include <cassert>
#include <functional>
#include <iostream>

void f(int a, int b) { std::cout << a << b; }
int g(int n) { return n + 1; }

int main() {
  using namespace std::placeholders;
  auto x = std::bind(f, _1, std::bind(g, _1));
  x(1);  // 12
}
```

* 绑定成员函数指针

```cpp
#include <cassert>
#include <functional>

struct A {
  int f(int n) const { return n; }
};

int main() {
  A a;
  auto x = std::bind(&A::f, &a, std::placeholders::_1);
  assert(x(42) == 42);
  auto y = std::bind(&A::f, a, std::placeholders::_1);
  assert(y(42) == 42);
}
```

* 绑定数据成员

```cpp
#include <cassert>
#include <functional>
#include <memory>

struct A {
  int i = 0;
};

int main() {
  auto x = std::bind(&A::i, std::placeholders::_1);
  A a;
  assert(x(a) == 1);
  assert(x(&a) == 1);
  assert(x(std::make_unique<A>(a)) == 1);
  assert(x(std::make_shared<A>(a)) == 1);
}
```

* 生成随机数

```cpp
#include <functional>
#include <iostream>
#include <random>

int main() {
  std::default_random_engine e;
  std::uniform_int_distribution<> d(0, 9);
  auto x = std::bind(d, e);
  for (int i = 0; i < 20; ++i) {
    std::cout << x();
  }
}
```

### [std::function](https://en.cppreference.com/w/cpp/utility/functional/function) 用法示例

* 存储函数对象

```cpp
#include <cassert>
#include <functional>

struct X {
  int operator()(int n) const { return n; }
};

int main() {
  std::function<int(int)> f = X();
  assert(f(0) == 0);
}
```

* 存储 bind 对象

```cpp
#include <cassert>
#include <functional>

int f(int i) { return i; }

int main() {
  std::function<int(int)> g = std::bind(f, std::placeholders::_1);
  assert(g(0) == 0);
}
```

* 存储绑定成员函数指针的 bind 对象

```cpp
#include <cassert>
#include <functional>

struct A {
  int f(int n) const { return n; }
};

int main() {
  A a;
  std::function<int(int)> g = std::bind(&A::f, &a, std::placeholders::_1);
  assert(g(0) == 0);
}
```

* 存储成员函数

```cpp
#include <cassert>
#include <functional>

struct A {
  int f(int n) const { return n; }
};

int main() {
  std::function<int(A&, int)> g1 = &A::f;
  A a;
  assert(g1(a, 0) == 0);

  std::function<int(const A&, int)> g2 = &A::f;
  const A b;
  assert(g2(b, 0) == 0);
}
```

* 存储数据成员

```cpp
#include <cassert>
#include <functional>

struct A {
  int i = 0;
};

int main() {
  std::function<int(const A&)> g = &A::i;
  A a;
  assert(g(a) == 0);
}
```
