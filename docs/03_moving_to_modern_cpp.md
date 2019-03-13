## 07 创建对象时注意区分 () 和 {}

* 值初始化有如下方式

```cpp
int a(0);
int b = 0;
int c{0};
int d = {0};  // 按 int d{0} 处理，后续讨论将忽略这种用法
```

* 使用等号不一定是赋值，也可能是拷贝，对于内置类型来说，初始化和赋值的区别只是学术争议，但对于类类型则不同

```cpp
A a;      // 默认构造
A b = a;  // 拷贝而非赋值
a = b;    // 拷贝而非赋值
```

* C++11 引入了统一初始化（uniform initialization），也可以叫大括号初始化（braced initialization）。大括号初始化可以方便地为容器指定初始元素

```cpp
std::vector<int> v{1, 2, 3};
```

* 可以用大括号初始化或 = 为 non-static 数据成员指定默认值，但不能用小括号初始化指定

```cpp
struct A {
  int x{0};   // OK
  int y = 0;  // OK
  int z(0);   // 错误
};
```

* 大括号初始化禁止内置类型的隐式收缩转换（implicit narrowing conversions），而小括号初始化和 = 不会

```cpp
double x = 1.1;
double y = 2.2;
int a{x + y};  // 错误：大括号初始化不允许 double 到 int 的收缩转换
int b(x + y);   // OK：double 被截断为 int
int c = x + y;  // OK：double 被截断为 int
```

* 大括号初始化不存在 C++ 的最令人苦恼的解析（C++'s most vexing parse）问题

```cpp
struct A {
  A() { std::cout << 1; }
};

struct B {
  B(std::string) { std::cout << 2; }
};

A a();  // 不调用 A 的构造函数，而是被解析成一个函数声明：A a();
std::string s{"hi"};
B b(std::string(s));  // 不构造 B，被解析成函数声明 B b(std::string)
A a2{};               // 构造 A
B b2{std::string(s)};  // 构造 B

// C++11 之前的解决办法
A a3;
B b3((std::string(s)));
```

* 大括号初始化的缺陷在于，只要类型转换后可以匹配，大括号初始化总会优先匹配参数类型为 [std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list) 的构造函数，即使收缩转换会导致调用错误

```cpp
#include <iostream>
#include <string>

struct A {
  A(int) { std::cout << 1; }
  A(std::string) { std::cout << 2; }
  A(std::initializer_list<int>) { std::cout << 3; }
};

int main() {
  A a{0};  // 3
  // A b{3.14};  // 错误：大括号初始化不允许 double 到 int 的收缩转换
  A c{"hi"};  // 2
}
```

* 但特殊的是，参数为空的大括号初始化只会调用默认构造函数。如果想传入真正的空 [std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list) 作为参数，则要额外添加一层大括号或小括号

```cpp
#include <iostream>

struct A {
  A() { std::cout << 1; }
  A(std::initializer_list<int>) { std::cout << 2; }
};

int main() {
  A a{};    // 1
  A b{{}};  // 2
  A c({});  // 2
}
```

* 上述问题带来的实际影响很大，比如 [std::vector](https://en.cppreference.com/w/cpp/container/vector) 就存在参数为参数 [std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list) 的构造函数，这导致了参数相同时，大括号初始化和小括号初始化调用的却是不同版本的构造函数

```cpp
std::vector<int> v1(3, 6);  // 元素为 6、6、6
std::vector<int> v2{3, 6};  // 元素为 3、6
```

* 这是一种失败的设计，并给模板作者带来了对大括号初始化和小括号初始化的选择困惑

```cpp
template <typename T, typename... Ts>
decltype(auto) f(Ts&&... args) {
  T x(std::forward<Ts>(args)...);  // 用小括号初始化创建临时对象
  return x;
}

template <typename T, typename... Ts>
decltype(auto) g(Ts&&... args) {
  T x{std::forward<Ts>(args)...};  // 用大括号初始化创建临时对象
  return x;
}

// 模板作者不知道调用者希望得到哪个结果
auto v1 = f<std::vector<int>>(3, 6);  // v1 元素为 6、6、6
auto v2 = g<std::vector<int>>(3, 6);  // v2 元素为 3、6
```

* [std::make_shared](https://en.cppreference.com/w/cpp/memory/shared_ptr/make_shared) 和 [std::make_unique](https://en.cppreference.com/w/cpp/memory/unique_ptr/make_unique) 就面临了这个问题，而它们的选择是使用小括号初始化并在接口文档中写明这点

```cpp
#include <iostream>
#include <vector>

int main() {
  auto p = std::make_shared<std::vector<int>>(3, 6);
  for (auto x : *p) {
    std::cout << x;  // 666
  }
}
```

## 08 用 [nullptr](https://en.cppreference.com/w/cpp/language/nullptr) 替代 0 和 [NULL](https://en.cppreference.com/w/cpp/types/NULL)

* 字面值 0 本质是 int 而非指针，只有在使用指针的语境中发现 0 才会解释为空指针
* [NULL](https://en.cppreference.com/w/cpp/types/NULL) 的本质是宏，没有规定的实现标准，一般在 C++ 中定义为 0，在 C 中定义为 `void*`

```cpp
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif
```

* 在重载解析时，[NULL](https://en.cppreference.com/w/cpp/types/NULL) 作为参数不会优先匹配指针类型。而 [nullptr](https://en.cppreference.com/w/cpp/language/nullptr) 的类型是 [std::nullptr_t](https://en.cppreference.com/w/cpp/types/nullptr_t)，[std::nullptr_t](https://en.cppreference.com/w/cpp/types/nullptr_t) 可以转换为任何原始指针类型

```cpp
#include <cstddef>

namespace jc {

constexpr int f(bool) { return 1; }
constexpr int f(int) { return 2; }
constexpr int f(void*) { return 3; }

static_assert(f(0) == 2);
static_assert(f(NULL) == 2);
static_assert(f(nullptr) == 3);

}  // namespace jc

int main() {}
```

* 这点也会影响模板实参推断

```cpp
template <typename T>
void f() {}

f(0);        // T 推断为 int
f(NULL);     // T 推断为 int
f(nullptr);  // T 推断为 std::nullptr_t
```

* 使用 [nullptr](https://en.cppreference.com/w/cpp/language/nullptr) 就可以避免推断出非指针类型

```cpp
void f1(std::shared_ptr<int>) {}
void f2(std::unique_ptr<int>) {}
void f3(int*) {}

template <typename F, tpyename T>
void g(F f, T x) {
  f(x);
}

g(f1, 0);        // 错误
g(f1, NULL);     // 错误
g(f1, nullptr);  // OK

g(f2, 0);        // 错误
g(f2, NULL);     // 错误
g(f2, nullptr);  // OK

g(f3, 0);        // 错误
g(f3, NULL);     // 错误
g(f3, nullptr);  // OK
```

## 09 用 [using 别名声明](https://en.cppreference.com/w/cpp/language/type_alias)替代 [typedef](https://en.cppreference.com/w/cpp/language/typedef)

* [using 别名声明](https://en.cppreference.com/w/cpp/language/type_alias)比 [typedef](https://en.cppreference.com/w/cpp/language/typedef) 可读性更好，尤其是对于函数指针类型

```cpp
using F = void (*)(int);  // typedef void (*F)(int)
```

* C++11 还引入了[别名模板](https://en.cppreference.com/w/cpp/language/type_alias)，它只能使用 [using 别名声明](https://en.cppreference.com/w/cpp/language/type_alias)

```cpp
template <typename T>
using Vector = std::vector<T>;  // Vector<int> 等价于 std::vector<int>

// C++11 之前的做法是在模板内部 typedef
template <typename T>
struct V {  // V<int>::type 等价于 std::vector<int>
  typedef std::vector<T> type;
};

// 在其他类模板中使用这两个别名的方式
template <typename T>
struct A {
  Vector<T> a;
  typename V<T>::type b;
};
```

* C++11 引入了 [type traits](https://en.cppreference.com/w/cpp/header/type_traits)，为了方便使用，C++14 为每个 [type traits](https://en.cppreference.com/w/cpp/header/type_traits) 都定义了[别名模板](https://en.cppreference.com/w/cpp/language/type_alias)

```cpp
template <typename T>
struct remove_reference {
  using type = T;
};

template <typename T>
struct remove_reference<T&> {
  using type = T;
};

template <typename T>
struct remove_reference<T&&> {
  using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;
```

* 为了简化生成值的 [type traits](https://en.cppreference.com/w/cpp/header/type_traits)，C++14 还引入了[变量模板](https://en.cppreference.com/w/cpp/language/variable_template)

```cpp
template <typename T, tpyename U>
struct is_same {
  static constexpr bool value = false;
};

template <typename T>
constexpr bool is_same_v = is_same<T, U>::value;
```

## 10 用 [enum class](https://en.cppreference.com/w/cpp/language/enum#Scoped_enumerations) 替代 [enum](https://en.cppreference.com/w/cpp/language/enum#Unscoped_enumeration)

* 一般在大括号中声明的名称，只在大括号的作用域内可见，但这对 enum 成员例外。enum 成员属于 enum 所在的作用域，因此作用域内不能出现同名实例

```cpp
enum X { a, b, c };
int a = 1;  // 错误：a 已在作用域内声明过
```

* C++11 引入了限定作用域的枚举类型，用 enum class 关键字表示

```cpp
enum class X { a, b, c };
int a = 1;   // OK
X x = X::a;  // OK
X y = b;     // 错误
```

* enum class 的另一个优势是不会进行隐式转换

```cpp
enum X { a, b, c };
X x = a;
if (x < 3.14) {  // 不应该将枚举与浮点数进行比较，但这里合法
}

enum class Y { a, b, c };
Y y = Y::a;
if (x < 3.14) {  // 错误：不允许比较
}
if (static_cast<double>(x) < 3.14) {  // OK：enum class 允许强制转换为其他类型
}
```

* C++11 之前的 enum 不允许前置声明，而 C++11 的 enum 和 enum class 都可以前置声明

```cpp
enum Color;    // C++11 之前错误
enum class X;  // OK
```

* C++11 之前不能前置声明 enum 的原因是，编译器为了节省内存，要在 enum 被使用前选择一个足够容纳成员取值的最小整型作为底层类型

```cpp
enum X { a, b, c };  // 编译器选择底层类型为 char
enum Status {        // 编译器选择比 char 更大的底层类型
  good = 0,
  failed = 1,
  incomplete = 100,
  corrupt = 200,
  indeterminate = 0xFFFFFFFF
};
```

* 不能前置声明的一个弊端是，由于编译依赖关系，在 enum 中仅仅添加一个成员可能就要重新编译整个系统。如果在头文件中包含前置声明，修改 enum class 的定义时就不需要重新编译整个系统，如果 enum class 的修改不影响函数的行为，则函数的实现也不需要重新编译
* C++11 支持前置声明的原因很简单，底层类型是已知的，用 [std::underlying_type](https://en.cppreference.com/w/cpp/types/underlying_type) 即可获取。也可以指定枚举的底层类型，如果不指定，enum class 默认为 int，enum 则不存在默认类型

```cpp
enum class X : std::uint32_t;
// 也可以在定义中指定
enum class Y : std::uint32_t { a, b, c };
```

* C++11 中使用 enum 更方便的场景只有一种，即希望用到 enum 的隐式转换时

```cpp
enum X { name, age, number };
auto t = std::make_tuple("downdemo", 6, "42");
auto x = std::get<name>(t);  // name 可隐式转换为 get 的模板参数类型 size_t
```

* 如果用 enum class，则需要强制转换

```cpp
enum class X { name, age, number };
auto t = std::make_tuple("downdemo", 6, "13312345678");
auto x = std::get<static_cast<std::size_t>(X::name)>(t);
```

* 可以用一个函数来封装转换的过程，但也不会简化多少

```cpp
template <typename E>
constexpr auto f(E e) noexcept {
  return static_cast<std::underlying_type_t<E>>(e);
}

auto x = std::get<f(X::name)>(t);
```

## 11 用 =delete 替代 private 作用域来禁用函数

* C++11 之前禁用拷贝的方式是将拷贝构造函数和拷贝赋值运算符声明在 private 作用域中

```cpp
class A {
 private:
  A(const A&);  // 不需要定义
  A& operator(const A&);
};
```

* C++11 中可以直接将要删除的函数用 =delete 声明，习惯上会声明在 public 作用域中，这样在使用删除的函数时，会先检查访问权再检查删除状态，出错时能得到更明确的诊断信息

```cpp
class A {
 public:
  A(const A&) = delete;
  A& operator(const A&) = delete;
};
```

* private 作用域中的函数还可以被成员和友元调用，而 =delete 是真正禁用了函数，无法通过任何方法调用
* 任何函数都可以用 =delete 声明，比如函数不想接受某种类型的参数，就可以删除对应类型的重载

```cpp
void f(int);
void f(double) = delete;  // 拒绝 double 和 float 类型参数

f(3.14);  // 错误
```

* =delete 还可以禁止模板对某个类型的实例化

```cpp
template <typename T>
void f(T x) {}

template <>
void f<int>(int) = delete;

f(1);  // 错误：使用已删除的函数
```

* 类内的函数模板也可以用这种方式禁用

```cpp
class A {
 public:
  template <typename T>
  void f(T x) {}
};

template <>
void A::f<int>(int) = delete;
```

* 当然，写在 private 作用域也可以起到禁用的效果

```cpp
class A {
 public:
  template <typename T>
  void f(T x) {}

 private:
  template <>
  void f<int>(int);
};
```

* 但把模板和特化置于不同的作用域不太合逻辑，与其效仿 =delete 的效果，不如直接用 =delete

## 12 用 [override](https://en.cppreference.com/w/cpp/language/override) 标记被重写的虚函数

* 虚函数的重写（override）很容易出错，因为要在派生类中重写虚函数，必须满足一系列要求
  * 基类中必须有此虚函数
  * 基类和派生类的函数名相同（析构函数除外）
  * 函数参数类型相同
  * const 属性相同
  * 函数返回值和异常说明相同
* C++11 多出一条要求：引用修饰符相同。引用修饰符的作用是，指定成员函数仅在对象为左值（成员函数标记为 &）或右值（成员函数标记为 &&）时可用

```cpp
namespace jc {

struct A {
  constexpr int f() & { return 1; }   // *this 是左值时才使用
  constexpr int f() && { return 2; }  // *this 是右值时才使用
};

constexpr A make_a() { return A{}; }

}  // namespace jc

int main() {
  jc::A a;
  static_assert(a.f() == 1);
  static_assert(jc::make_a().f() == 2);
}
```

* 对于这么多的要求难以面面俱到，比如下面代码没有任何重写但可以通过编译

```cpp
struct A {
 public:
  virtual void f1() const;
  virtual void f2(int x);
  virtual void f3() &;
  void f4() const;
};

struct B : A {
  virtual void f1();
  virtual void f2(unsigned int x);
  virtual void f3() &&;
  void f4() const;
};
```

* 为了保证正确性，C++11 提供了 [override](https://en.cppreference.com/w/cpp/language/override) 来标记要重写的虚函数，如果未重写就不能通过编译

```cpp
struct A {
  virtual void f1() const;
  virtual void f2(int x);
  virtual void f3() &;
  virtual void f4() const;
};

struct B : A {
  virtual void f1() const override;
  virtual void f2(int x) override;
  virtual void f3() & override;
  void f4() const override;
};
```

* [override](https://en.cppreference.com/w/cpp/language/override) 是一个 contextual keyword，只在特殊语境中保留，[override](https://en.cppreference.com/w/cpp/language/override) 只有出现在成员函数声明末尾才有保留意义，因此如果以前的遗留代码用到了 [override](https://en.cppreference.com/w/cpp/language/override) 作为名字，不用改名就可以升到 C++11

```cpp
struct A {
  void override();  // 在 C++98 和 C++11 中都合法
};
```

* C++11 还提供了另一个 contextual keyword，即 [final](https://en.cppreference.com/w/cpp/language/final)，它可以用来指定虚函数禁止被重写

```cpp
struct A {
  virtual void f() final;
  void g() final;  // 错误：final 只能用于指定虚函数
};

struct B : A {
  virtual void f() override;  // 错误：f 不可重写
};
```

* [final](https://en.cppreference.com/w/cpp/language/final) 还可以用于指定某个类禁止被继承

```cpp
struct A final {};
struct B : A {};  // 错误：A 禁止被继承
```

## 13 用 [std::cbegin](https://en.cppreference.com/w/cpp/iterator/begin) 和 [std::cend](https://en.cppreference.com/w/cpp/iterator/end) 获取 const_iterator

* 需要迭代器但不修改值时就应该使用 const_iterator，获取和使用 const_iterator 十分简单

```cpp
std::vector<int> v{2, 3};
auto it = std::find(std::cbegin(v), std::cend(v), 2);  // C++14
v.insert(it, 1);
```

* 上述功能很容易扩展成模板

```cpp
template <typename C, typename T>
void f(C& c, const T& x, const T& y) {
  auto it = std::find(std::cbegin(c), std::cend(c), x);
  c.insert(it, y);
}
```

* C++11 没有 [std::cbegin](https://en.cppreference.com/w/cpp/iterator/begin) 和 [std::cend](https://en.cppreference.com/w/cpp/iterator/end)，手动实现即可

```cpp
template <typename C>
auto cbegin(const C& c) -> decltype(std::begin(c)) {
  return std::begin(c);  // c 是 const 所以返回 const_iterator
}
```

## 14 用 [noexcept](https://en.cppreference.com/w/cpp/language/noexcept_spec) 标记不抛异常的函数

* C++98 中，必须指出一个函数可能抛出的所有异常类型，如果函数有所改动则 [exception specification](https://en.cppreference.com/w/cpp/language/except_spec) 也要修改，而这可能破坏代码，因为调用者可能依赖于原本的 [exception specification](https://en.cppreference.com/w/cpp/language/except_spec)，所以 C++98 中的 [exception specification](https://en.cppreference.com/w/cpp/language/except_spec) 被认为不值得使用
* C++11 中达成了一个共识，真正需要关心的是函数会不会抛出异常。一个函数要么可能抛出异常，要么绝对不抛异常，这种 maybe-or-never 形成了 C++11 [exception specification](https://en.cppreference.com/w/cpp/language/except_spec) 的基础，C++98 的 [exception specification](https://en.cppreference.com/w/cpp/language/except_spec) 在 C++17 移除
* 函数是否要加上 noexcept 声明与接口设计相关，调用者可以查询函数的 noexcept 状态，查询结果将影响代码的异常安全性和执行效率。因此函数是否要声明为 noexcept 就和成员函数是否要声明为 const 一样重要，如果一个函数不抛异常却不为其声明 noexcept，这就是接口规范缺陷
* noexcept 的一个额外优点是，它可以让编译器生成更好的目标代码。为了理解原因只需要考虑 C++98 和 C++11 表达函数不抛异常的区别

```cpp
int f(int x) throw();   // C++98
int f(int x) noexcept;  // C++11
```

* 如果一个异常在运行期逃出函数，则 [exception specification](https://en.cppreference.com/w/cpp/language/except_spec) 被违反。在 C++98 中，调用栈会展开到函数调用者，执行一些无关的动作后中止程序。C++11 的一个微小区别是是，在程序中止前只是可能而非一定展开栈。这一点微小的区别将对代码生成造成巨大的影响
* noexcept 声明的函数中，如果异常传出函数，优化器不需要保持栈在运行期的展开状态，也不需要在异常逃出时，保证其中所有的对象按构造顺序的逆序析构。而声明为 throw() 的函数就没有这样的优化灵活性。总结起来就是

```cpp
RetType function(params) noexcept;  // most optimizable
RetType function(params) throw();   // less optimizable
RetType function(params);           // less optimizable
```

* 这个理由已经足够支持给任何已知不会抛异常的函数加上 noexcept，比如移动操作就是典型的不抛异常函数
* [std::vector::push_back](https://en.cppreference.com/w/cpp/container/vector/push_back) 在容器空间不够容纳元素时，会扩展新的内存块，再把元素转移到新的内存块。C++98 的做法是逐个拷贝，然后析构旧内存的对象，这使得 [push_back](https://en.cppreference.com/w/cpp/container/vector/push_back) 提供强异常安全保证：如果拷贝元素的过程中抛出异常，则 [std::vector](https://en.cppreference.com/w/cpp/container/vector) 保持原样，因为旧内存元素还未被析构
* [std::vector::push_back](https://en.cppreference.com/w/cpp/container/vector/push_back) 在 C++11 中的优化是把拷贝替换成移动，但为了不违反强异常安全保证，只有确保元素的移动操作不抛异常时才会用移动替代拷贝
* swap 函数是需要 noexcept 声明的另一个例子，不过标准库的 swap 用 [noexcept 操作符](https://en.cppreference.com/w/cpp/language/noexcept)的结果决定

```cpp
/*
 * 数组的 swap 由元素类型决定 noexcept 结果
 * 比如元素类型是 class A
 * 如果 swap(A, A) 不抛异常则该数组的 swap 也不抛异常
 */
template <typename T, size_t N>
void swap(T (&a)[N], T (&b)[N]) noexcept(noexcept(swap(*a, *b)));

// std::pair 的 swap
template <typename T, typename U>
struct pair {
  void swap(pair& p) noexcept(
      noexcept(swap(first, p.first)) && noexcept(swap(second, p.second)));
};
```

* 虽然 noexcept 有优化的好处，但将函数声明为 noexcept 的前提是，保证函数长期具有 noexcept 性质，如果之后随意移除 noexcept 声明，就有破坏客户代码的风险
* 大多数函数是异常中立的，它们本身不抛异常，但它们调用的函数可能抛异常，这样它们就允许抛出的异常传到调用栈的更深一层，因此异常中立函数天生永远不具备 noexcept 性质
* 如果为了强行加上 noexcept 而修改实现就是本末倒置，比如调用一个会抛异常的函数是最简单的实现，为了不抛异常而环环相扣地来隐藏这点（比如捕获所有异常，将其替换成状态码或特殊返回值），大大增加了理解和维护的难度，并且这些复杂性的时间成本可能超过 noexcept 带来的优化
* 对某些函数来说，noexcept 性质十分重要，内存释放函数和所有的析构函数都隐式 noexcept，这样就不必加 noexcept 声明。析构函数唯一未隐式 noexcept 的情况是，类中有数据成员的类型显式将析构函数声明 noexcept(false)。但这样的析构函数很少见，标准库中一个也没有
* 有些库的接口设计者会把函数区分为 wide contract 和 narrow contract
* wide contract 函数没有前置条件，不用关心程序状态，对传入的实参没有限制，一定不会有未定义行为，如果知道不会抛异常就可以加上 noexcept
* narrow contract 函数有前置条件，如果条件被违反则结果未定义。但函数没有义务校验这个前置条件，它断言前置条件一定满足（调用者负责保证断言成立），因此加上 noexcept 声明也是合理的

```cpp
// 假设前置条件是 s.size() <= 32
void f(const std::string& s) noexcept;
```

* 但如果想在违反前置条件时抛出异常，由于函数的 noexcept 声明，异常就会导致程序中止，因此一般只为 wide contract 函数声明 noexcept
* 在 noexcept 函数中调用可能抛异常的函数时，编译器不会帮忙给出警告。带 noexcept 声明的函数调用了不带 noexcept 声明的函数，这看起来自相矛盾，但也许被调用的函数在文档中写明了不会抛异常，也许它们来自 C 语言的库，也许来自还没来得及根据 C++11 标准做修订的 C++98 库

## 15 用 [constexpr](https://en.cppreference.com/w/cpp/language/constexpr) 表示编译期常量

* constexpr 用于对象时就是一个加强版的 const，表面上看 constexpr 表示值是 const，且在编译期（严格来说是翻译期，包括编译和链接，如果不是编译器或链接器作者，无需关心这点区别）已知，但用于函数则有不同的意义
* 编译期已知的值可能被放进只读内存，这对嵌入式开发是一个很重要的语法特性
* constexpr 函数在调用时若传入的是编译期常量，则产出编译期常量，传入运行期才知道的值，则产出运行期值。constexpr 函数可以满足所有需求，因此不必为了有非编译期值的情况而写两个函数

```cpp
#define CPP98 199711L
#define CPP11 201103L
#define CPP14 201402L
#define CPP17 201703L
#define CPP20 202002L

// #if ((defined(_MSVC_LANG) && _MSVC_LANG > CPP11) || __cplusplus > CPP11)
// #define JC_HAS_CXX14
// #endif

#ifndef JC_HAS_CXX14
#ifdef _MSVC_LANG
#if _MSVC_LANG > CPP11
#define JC_HAS_CXX14 1
#else
#define JC_HAS_CXX14 0
#endif
#else
#if __cplusplus > CPP11
#define JC_HAS_CXX14 1
#else
#define JC_HAS_CXX14 0
#endif
#endif
#endif  // JC_HAS_CXX14

namespace jc {

constexpr int pow(int base, int exp) noexcept {
#ifdef JC_HAS_CXX14
  auto res = 1;
  for (int i = 0; i < exp; ++i) {
    res *= base;
  }
  return res;
#else  // C++11 中，constexpr 函数只能包含一条语句
  return (exp == 0 ? 1 : base * pow(base, exp - 1));
#endif
}

}  // namespace jc

int main() {
  constexpr auto n = 4;
  static_assert(jc::pow(3, n) == 81);
}
```

* constexpr 并不表示函数要返回 const 值，而是表示，如果参数都是编译期常量，则返回结果就可以当编译期常量使用，如果有一个不是编译期常量，返回值就在运行期计算

```cpp
auto base = 3;                    // 运行期获取值
auto exp = 10;                    // 运行期获取值
auto baseToExp = pow(base, exp);  // pow 在运行期被调用
```

* constexpr 函数必须传入和返回 [literal type](https://en.cppreference.com/w/cpp/named_req/LiteralType)。constexpr 构造函数可以让自定义类型也成为 [literal type](https://en.cppreference.com/w/cpp/named_req/LiteralType)

```cpp
namespace jc {

class Point {
 public:
  constexpr Point(double x = 0, double y = 0) noexcept : x_(x), y_(y) {}
  constexpr double x_value() const noexcept { return x_; }
  constexpr double y_value() const noexcept { return y_; }
  void set_x(double x) noexcept {
    x_ = x;  // 修改了对象所以不能声明为 constexpr
  }
  void set_y(double y) noexcept {  //  C++11 的 constexpr 函数不能返回 void
    y_ = y;
  }

 private:
  double x_;
  double y_;
};

constexpr Point midpoint(const Point& lhs, const Point& rhs) noexcept {
  return {(lhs.x_value() + rhs.x_value()) / 2,
          (lhs.y_value() + rhs.y_value()) / 2};
}

}  // namespace jc

int main() {
  constexpr jc::Point p1{1.1, 2.2};  // 编译期执行 constexpr 构造函数
  constexpr jc::Point p2{3.3, 4.4};  // 同上
  constexpr auto mid = jc::midpoint(p1, p2);
  static_assert(mid.x_value() == (1.1 + 3.3) / 2);
  static_assert(mid.y_value() == (2.2 + 4.4) / 2);
}
```

* C++14 允许对值进行了修改或无返回值的函数声明为 constexpr

```cpp
namespace jc {

class Point {
 public:
  constexpr Point(double x = 0, double y = 0) noexcept : x_(x), y_(y) {}
  constexpr double x_value() const noexcept { return x_; }
  constexpr double y_value() const noexcept { return y_; }
  constexpr void set_x(double x) noexcept { x_ = x; }
  constexpr void set_y(double y) noexcept { y_ = y; }

 private:
  double x_;
  double y_;
};

constexpr Point midpoint(const Point& lhs, const Point& rhs) noexcept {
  return {(lhs.x_value() + rhs.x_value()) / 2,
          (lhs.y_value() + rhs.y_value()) / 2};
}

constexpr Point reflection(const Point& p) noexcept {  // p 关于原点的对称点
  Point res;
  res.set_x(-p.x_value());
  res.set_y(-p.y_value());
  return res;
}

}  // namespace jc

int main() {
  constexpr jc::Point p1{1.1, 2.2};
  constexpr jc::Point p2{3.3, 4.4};
  constexpr auto mid = jc::midpoint(p1, p2);
  static_assert(mid.x_value() == (1.1 + 3.3) / 2);
  static_assert(mid.y_value() == (2.2 + 4.4) / 2);
  constexpr auto reflection_mid = jc::reflection(mid);
  static_assert(reflection_mid.x_value() == -mid.x_value());
  static_assert(reflection_mid.y_value() == -mid.y_value());
}
```

* 使用 constexpr 的前提是必须长期保证需要它，因为如果后续要删除 constexpr 可能会导致许多错误

## 16 用 [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex) 或 [std::atomic](https://en.cppreference.com/w/cpp/atomic/atomic) 保证 const 成员函数线程安全

* 假设有一个表示多项式的类，它包含一个返回根的 const 成员函数

```cpp
class Polynomial {
 public:
  std::vector<double> roots() const {
    if (!roots_are_valid_) {
      // 计算 root_vals_
      roots_are_valid_ = true;
    }
    return root_vals_;
  }

 private:
  mutable bool roots_are_valid_{false};
  mutable std::vector<double> root_vals_{};
};
```

* 假如此时有两个线程对同一个对象调用成员函数，虽然函数声明为 const，但由于函数内部修改了数据成员，就可能产生数据竞争。最简单的解决方法是引入一个 [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex)

```cpp
class Polynomial {
 public:
  std::vector<double> roots() const {
    std::lock_guard<std::mutex> lk{m_};
    if (!roots_are_valid_) {
      // 计算 root_vals_
      roots_are_valid_ = true;
    }
    return root_vals_;
  }

 private:
  mutable std::mutex m_;
  mutable bool roots_are_valid_{false};
  mutable std::vector<double> root_vals_{};
};
```

* 对一些简单的情况，使用原子变量 [std::atomic](https://en.cppreference.com/w/cpp/atomic/atomic) 可能开销更低（取决于机器及 [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex) 的实现）

```cpp
class Point {
 public:
  double distance_from_origin() const noexcept {
    ++call_count_;  // 计算调用次数
    return std::sqrt((x_ * x_) + (y_ * y_));
  }

 private:
  mutable std::atomic<unsigned> call_count_{0};
  double x_;
  double y_;
};
```

* 因为 [std::atomic](https://en.cppreference.com/w/cpp/atomic/atomic) 的开销比较低，很容易想当然地用多个原子变量来同步

```cpp
class A {
 public:
  int f() const {
    if (flag_) {
      return res_;
    } else {
      auto x = expensive_computation1();
      auto y = expensive_computation2();
      res_ = x + y;
      flag_ = true;  // 设置标记
      return res_;
    }
  }

 private:
  mutable std::atomic<bool> flag_{false};
  mutable std::atomic<int> res_;
};
```

* 这样做可行，但如果多个线程同时观察到标记值为 false，每个线程都要继续进行运算，这个标记反而没起到作用。先设置标记再计算可以消除这个问题，但会引起一个更大的问题

```cpp
class A {
 public:
  int f() const {
    if (flag_) {
      return res_;
    } else {
      flag_ = true;  // 在计算前设置标记值为 true
      auto x = expensive_computation1();
      auto y = expensive_computation2();
      res_ = x + y;
      return res_;
    }
  }

 private:
  mutable std::atomic<bool> flag_{false};
  mutable std::atomic<int> res_;
};
```

* 假如线程 1 刚设置好标记，线程 2 此时正好检查到标记值为 true 并直接返回数据值，然后线程 1 接着计算结果，这样线程 2 的返回值就是错的
* 因此如果要同步多个变量或内存区，最好还是使用 [std::mutex](https://en.cppreference.com/w/cpp/thread/mutex)

```cpp
class A {
 public:
  int f() const {
    std::lock_guard<std::mutex> lk{m_};
    if (flag_) {
      return res_;
    } else {
      auto x = expensive_computation1();
      auto y = expensive_computation2();
      res_ = x + y;
      flag_ = true;
      return res_;
    }
  }

 private:
  mutable std::mutex m_;
  mutable bool flag_{false};
  mutable int res_;
};
```

## 17 特殊成员函数的隐式合成与抑制机制

* C++11 中的特殊成员函数多了两个：移动构造函数和移动赋值运算符

```cpp
struct A {
  A(A&& rhs);             // 移动构造函数
  A& operator=(A&& rhs);  // 移动赋值运算符
};
```

* 移动操作同样会在需要时生成，执行的是对 non-static 成员的移动操作，另外它们也会对基类部分执行移动操作
* 移动操作并不确保真正移动，其核心是把 [std::move](https://en.cppreference.com/w/cpp/utility/move) 用于每个要移动的对象，根据返回值的重载解析决定执行移动还是拷贝。因此按成员移动分为两部分：对支持移动操作的类型进行移动，对不可移动的类型执行拷贝
* 两种拷贝操作（拷贝构造函数和拷贝复制运算符）是独立的，声明其中一个不会阻止编译器生成另一个
* 两种移动操作是不独立的，声明其中一个将阻止编译器生成另一个。理由是如果声明了移动构造函数，可能意味着实现上与编译器默认按成员移动的移动构造函数有所不同，从而可以推断移动赋值操作也应该与默认行为不同
* 显式声明拷贝操作（即使声明为 =delete）会阻止自动生成移动操作（但声明为 =default 不阻止生成）。理由类似上条，声明拷贝操作可能意味着默认的拷贝方式不适用，从而推断移动操作也应该会默认行为不同
* 反之亦然，声明移动操作也会阻止生成拷贝操作
* C++11 规定，显式声明析构函数会阻止生成移动操作。这个规定源于 Rule of Three，即两种拷贝函数和析构函数应该一起声明。这个规则的推论是，如果声明了析构函数，则说明默认的拷贝操作也不适用，但 C++98 中没有重视这个推论，因此仍可以生成拷贝操作，而在 C++11 中为了保持不破坏遗留代码，保留了这个规则。由于析构函数和拷贝操作需要一起声明，加上声明了拷贝操作会阻止生成移动操作，于是 C++11 就有了这条规定
* 最终，生成移动操作的条件必须满足：该类没有用户声明的拷贝、移动、析构中的任何一个函数
* 总有一天这个规则会扩展到拷贝操作，因为 C++11 规定存在拷贝操作或析构函数时，仍能生成拷贝操作是被废弃的行为。C++11 提供了 =default 来表示使用默认行为，而不抑制生成其他函数
* 这种手法对于多态基类很有用，多态基类一般会有虚析构函数，虚析构函数的默认实现一般是正确的，为了使用默认行为而不阻止生成移动操作，则应该使用 =default，同理，如果要使用默认的移动操作而不阻止生成拷贝操作，则应该给移动操作加上 =default

```cpp
struct A {
  virtual ~A() = default;
  A(A&&) = default;
  A& operator=(A&&) = default;
  A(const A&) = default;
  A& operator=(const A&) = default;
};
```

* 事实上不需要思考太多限制，如果需要默认操作就使用 =default，虽然麻烦一些，但可以避免许多问题。对于如下类，没有声明任何特殊成员函数，编译器将在需要时自动合成

```cpp
class StringTable {
 private:
  std::map<int, std::string> values_;
};
```

* 假设过了一段时间后，想扩充一些行为，比如记录构造和析构日志

```cpp
class StringTable {
 public:
  StringTable() { makeLogEntry("Creating StringTable object"); }
  ~StringTable() { makeLogEntry("Destroying StringTable object"); }

 private:
  std::map<int, std::string> values_;
};
```

* 这时析构函数就会阻止生成移动操作，但针对移动操作的测试可以通过编译，因为在不可移动时会使用拷贝操作，而这很难被察觉。执行移动的代码实际变成了拷贝，而这一切只是源于添加了一个析构函数。避免这个问题也不是难事，只需要一开始把拷贝和移动操作声明为 =default
* 另外还有默认构造函数和析构函数的生成未被提及，这里将统一总结
  * 默认构造函数：和 C++98 相同，只在类中不存在用户声明的构造函数时生成
  * 析构函数：
    * 和 C++98 基本相同，唯一的区别是默认为 noexcept
    * 和 C++98 相同，只有基类的析构函数为虚函数，派生类的析构函数才为虚函数
  * 拷贝构造函数：
    * 仅当类中不存在用户声明的拷贝构造函数时生成
    * 如果声明了移动操作，则拷贝构造函数被删除
    * 如果声明了拷贝赋值运算符或析构函数，仍能生成拷贝构造函数，但这是被废弃的行为
  * 拷贝赋值运算符：
    * 仅当类中不存在用户声明的拷贝赋值运算符时生成
    * 如果声明了移动操作，则拷贝赋值运算符被删除
    * 如果声明了拷贝构造函数或析构函数，仍能生成拷贝赋值运算符，但这是被废弃的行为
  * 移动操作：仅当类中不存在任何用户声明的拷贝操作、移动操作、析构函数时生成
* 注意，这些机制中提到的是成员函数而非成员函数模板，模板并不会影响特殊成员函数的合成

```cpp
struct A {
  template <typename T>
  A(const T& rhs);  // 从任意类型构造

  template <typename T>
  A& operator=(const T& rhs);  // 从任意类型赋值
};
```

* 上述模板不会阻止编译器生成拷贝和移动操作，即使模板的实例化和拷贝操作签名相同（即 T 是 A）
