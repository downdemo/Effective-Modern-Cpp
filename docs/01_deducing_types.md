## 01 模板类型推断机制

* auto 推断的基础是模板类型推断机制，但部分特殊情况下，模板推断机制不适用于 auto
* 模板的形式可以看成如下伪代码

```cpp
template <typename T>
void f(ParamType x);  // ParamType 即 x 的类型
```

* 调用可看成

```cpp
f(expr);
```

* 编译期间，编译器用 expr 推断 T 和 ParamType，实际上两者通常不一致，比如

```cpp
template <typename T>
void f(const T& x);

int x;  // 为方便演示，只指定类型不初始化，后续同理
f(x);   // T 被推断为 int，ParamType 被推断为 const int&
```

* T 的类型推断与 expr 和 ParamType 相关

### 情形 1：ParamType 不是引用或指针

* 丢弃 expr 的 top-level cv 限定符和引用限定符，最后得到的 expr 类型就是 T 和 ParamType 的类型

```cpp
template <typename T>
void f(T x);

int a;
const int b;
const int& c;

int* p1;
const int* p2;
int* const p3;
const int* const p4;

char s1[] = "downdemo";
const char s2[] = "downdemo";

// 以下情况 T 和 ParamType 都是 int
f(a);
f(b);
f(c);
// 指针类型丢弃的是 top-level const（即指针本身的 const）
// low-level const（即所指对象的 const）会保留
f(p1);  // T 和 ParamType 都是 int*
f(p2);  // T 和 ParamType 都是 const int*
f(p3);  // T 和 ParamType 都是 int*
f(p4);  // T 和 ParamType 都是 const int*
// char 数组会退化为指针
f(s1);  // T 和 ParamType 都是 char*
f(s2);  // T 和 ParamType 都是 const char*
```

### 情形 2：ParamType 是引用类型

* 如果 expr 的类型是引用，保留 cv 限定符，ParamType 一定是左值引用类型，ParamType 去掉引用符就是 T 的类型，即 T 一定不是引用类型

```cpp
template <typename T>
void f(T& x);

int a;
int& b;
int&& c;
const int d;
const int& e;

int* p1;
const int* p2;
int* const p3;
const int* const p4;

char s1[] = "downdemo";
const char s2[] = "downdemo";

f(a);  // ParamType 是 int&，T 是 int
f(b);  // ParamType 是 int&，T 是 int
f(c);  // ParamType 是 int&，T 是 int
f(d);  // ParamType 是 const int&，T 是 const int
f(e);  // ParamType 是 const int&，T 是 const int
// 因为 top-level const 和 low-level const 都保留
// 对于指针只要记住 const 的情况和实参类型一样
f(p1);  // ParamType 是 int* &，T 是 int*
f(p2);  // ParamType 是 const int* &，T 是 const int*
f(p3);  // ParamType 是 int* const&，T 是 int* const
f(p4);  // ParamType 是 const int* const &，T 是 const int* const
// 数组类型对于 T& 的情况比较特殊，不会退化到指针
f(s1);  // ParamType 是 char(&)[9]，T 是 char[9]
f(s2);  // ParamType 是 const char(&)[9]，T 是 const char[9]
```

* 如果把 ParamType 从 T& 改为 const T&，区别只是 ParamType 一定为 top-level const，ParamType 去掉 top-level const 和引用符就是 T 的类型，即 T 一定不为 top-level const 引用类型

```cpp
template <typename T>
void f(const T& x);

int a;
int& b;
int&& c;
const int d;
const int& e;

int* p1;
const int* p2;
int* const p3;
const int* const p4;

char s1[] = "downdemo";
const char s2[] = "downdemo";

// 以下情况 ParamType 都是 const int&，T 都是 int
f(a);
f(b);
f(c);
f(d);
f(e);
// 数组类型类似
f(s1);  // ParamType 是 const char(&)[9]，T 是 char[9]
f(s2);  // ParamType 是 const char(&)[9]，T 是 char[9]
// 对于指针只要记住，T 的指针符后一定无 const
f(p1);  // ParamType 是 int* const &，T 是 int*
f(p2);  // ParamType 是 const int* const &，T 是 const int*
f(p3);  // ParamType 是 int* const&，T 是 int*
f(p4);  // ParamType 是 const int* const &，T 是 const int*
```

* 对应数组类型的模板参数类型应声明为 `T(&)[N]`，即数组类型 `T[N]` 的引用

```cpp
namespace jc {

template <typename T, int N>
constexpr int f(T (&)[N]) noexcept {
  return N;
}

}  // namespace jc

int main() {
  const char s[] = "downdemo";
  static_assert(jc::f(s) == 9);
}
```

### 情形 3：ParamType 是指针类型

* 与情形 2 类似，ParamType 一定是 non-const 指针（传参时忽略 top-level const）类型，去掉指针符就是 T 的类型，即 T 一定不为指针类型

```cpp
template <typename T>
void f(T* x);

int a;
const int b;

int* p1;
const int* p2;
int* const p3;        // 传参时与 p1 类型一致
const int* const p4;  // 传参时与 p2 类型一致

char s1[] = "downdemo";
const char s2[] = "downdemo";

f(&a);  // ParamType 是 int*，T 是 int
f(&b);  // ParamType 是 const int*，T 是 const int

f(p1);  // ParamType 是 int*，T 是 int
f(p2);  // ParamType 是 const int*，T 是 const int
f(p3);  // ParamType 是 int*，T 是 int
f(p4);  // ParamType 是 const int*，T 是 const int

// 数组类型会转为指针类型
f(s1);  // ParamType 是 char*，T 是 char
f(s2);  // ParamType 是 const char*，T 是 const char
```

* 如果 ParamType 是 const-pointer，和上面实际上是同一个模板，ParamType 多出 top-level const，T 不变

```cpp
template <typename T>
void f(T* const x);

int a;
const int b;

int* p1;        // 传参时与 p3 类型一致
const int* p2;  // 传参时与 p4 类型一致
int* const p3;
const int* const p4;

char s1[] = "downdemo";
const char s2[] = "downdemo";

f(&a);  // ParamType 是 int* const，T 是 int
f(&b);  // ParamType 是 const int* const，T 是 const int

f(p1);  // ParamType 是 int* const，T 是 int
f(p2);  // ParamType 是 const int* const，T 是 const int
f(p3);  // ParamType 是 int* const，T 是 int
f(p4);  // ParamType 是 const int* const，T 是 const int

f(s1);  // ParamType 是 char* const，T 是 char
f(s2);  // ParamType 是 const char* const，T 是 const char
```

* 如果 ParamType 是 pointer to const，则只有一种结果，T 一定是不带 const 的非指针类型

```cpp
template <typename T>
void f(const T* x);

template <typename T>
void g(const T* const x);

int a;
const int b;

int* p1;
const int* p2;
int* const p3;
const int* const p4;

char s1[] = "downdemo";
const char s2[] = "downdemo";

// 以下情况 ParamType 都是 const int*，T 都是 int
f(&a);
f(&b);
f(p1);
f(p2);
f(p3);
f(p4);
// 以下情况 ParamType 都是 const int* const，T 都是 int
g(&a);
g(&b);
g(p1);
g(p2);
g(p3);
g(p4);
// 以下情况 ParamType 都是 const char*，T 都是 char
f(s1);
f(s2);
g(s1);
g(s2);
```

### 情形 4：ParamType 是转发引用

* 如果 expr 是左值，T 和 ParamType 都推断为左值引用。这有两点非常特殊
  * 这是 T 被推断为引用的唯一情形
  * ParamType 使用右值引用语法，却被推断为左值引用
* 如果 expr 是右值，则 ParamType 推断为右值引用类型，去掉 && 就是 T 的类型，即 T 一定不为引用类型

```cpp
template <typename T>
void f(T&& x);

int a;
const int b;
const int& c;
int&& d = 1;  // d 是右值引用，也是左值，右值引用是只能绑定右值的引用而不是右值

char s1[] = "downdemo";
const char s2[] = "downdemo";

f(a);  // ParamType 和 T 都是 int&
f(b);  // ParamType 和 T 都是 const int&
f(c);  // ParamType 和 T 都是 const int&
f(d);  // ParamType 和 T 都是 const int&
f(1);  // ParamType 是 int&&，T 是 int

f(s1);  // ParamType 和 T 都是 char(&)[9]
f(s2);  // ParamType 和 T 都是 const char(&)[9]
```

### 特殊情形：expr 是函数名

```cpp
template <typename T>
void f1(T x);

template <typename T>
void f2(T& x);

template <typename T>
void f3(T&& x);

void g(int);

f1(g);  // T 和 ParamType 都是 void(*)(int)
f2(g);  // ParamType 是 void(&)(int)，T 是 void()(int)
f3(g);  // T 和 ParamType 都是 void(&)(int)
```

## 02 [auto](https://en.cppreference.com/w/cpp/language/auto) 类型推断机制

* auto 类型推断几乎和模板类型推断一致
* 调用模板时，编译器根据 expr 推断 T 和 ParamType 的类型。当变量用 auto 声明时，auto 就扮演了模板中的 T 的角色，变量的类型修饰符则扮演 ParamType 的角色
* 为了推断变量类型，编译器表现得好比每个声明对应一个模板，模板的调用就相当于对应的初始化表达式

```cpp
auto x = 1;
const auto cx = x;
const auto& rx = x;

template <typename T>  // 用来推断 x 类型的概念上假想的模板
void func_for_x(T x);

func_for_x(1);  // 假想的调用: param 的推断类型就是 x 的类型

template <typename T>  // 用来推断 cx 类型的概念上假想的模板
void func_for_cx(const T x);

func_for_cx(x);  // 假想的调用: param 的推断类型就是 cx 的类型

template <typename T>  // 用来推断 rx 类型的概念上假想的模板
void func_for_rx(const T& x);

func_for_rx(x);  // 假想的调用: param 的推断类型就是 rx 的类型
```

* auto 的推断适用模板推断机制的三种情形：T&、T&& 和 T

```cpp
auto x = 1;          // int x
const auto cx = x;   // const int cx
const auto& rx = x;  // const int& rx
auto&& uref1 = x;    // int& uref1
auto&& uref2 = cx;   // const int& uref2
auto&& uref3 = 1;    // int&& uref3
```

* auto 对数组和指针的推断也和模板一致

```cpp
const char name[] = "downdemo";  // 数组类型是 const char[9]
auto arr1 = name;                // const char* arr1
auto& arr2 = name;               // const char (&arr2)[9]

void g(int, double);  // 函数类型是 void(int, double)
auto f1 = g;          // void (*f1)(int, double)
auto& f2 = g;         // void (&f2)(int, double)
```

* auto 推断唯一不同于模板实参推断的情形是 C++11 的初始化列表。下面是同样的赋值功能

```cpp
// C++98
int x1 = 1;
int x2(1);

// C++11
int x3 = {1};
int x4{1};
```

* 但换成 auto 声明，这些赋值的意义就不一样了

```cpp
auto x1 = 1;    // int x1
auto x2(1);     // int x2
auto x3 = {1};  // std::initializer_list<int> x3
auto x4{1};     // C++11 为 std::initializer_list<int> x4，C++14 为 int x4
```

* 如果初始化列表中元素类型不同，则无法推断

```cpp
auto x = {1, 2, 3.0};  // 错误：不能为 std::initializer_list<T> 推断 T
```

* C++14 禁止对 auto 用 [std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list) 直接初始化，而必须用  =，除非列表中只有一个元素，这时不会将其视为 [std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list)

```cpp
auto x1 = {1, 2};  // C++14 中必须用 =，否则报错
auto x2{1};  // 允许单元素的直接初始化，不会将其视为 initializer_list
```

* 模板不支持模板参数为 T 而 expr 为初始化列表的推断，不会将其假设为 [std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list)，这就是 auto 推断和模板推断唯一的不同之处

```cpp
auto x = {1, 2, 3};  // x 类型是 std::initializer_list<int>

template <typename T>  // 等价于 x 声明的模板
void f(T x);

f({1, 2, 3});  // 错误：不能推断 T 的类型
```

* 不过将模板参数为 [std::initializer_list](https://en.cppreference.com/w/cpp/utility/initializer_list) 则可以推断 T

```cpp
template <typename T>
void f(std::initializer_list<T> initList);

f({1, 2, 3});  // T 被推断为 int，initList 类型是 std::initializer_list<int>
```

### C++14 的 auto

* C++14 中，auto 可以作为函数返回类型，并且 lambda 可以将参数声明为 auto，这种 lambda 称为泛型 lambda

```cpp
auto f() { return 1; }
auto g = [](auto x) { return x; };
```

* 但此时 auto 仍然使用的是模板实参推断的机制，因此不能为 auto 返回类型返回一个初始化列表，即使是单元素

```cpp
auto f() { return {1}; }  // 错误
```

* 泛型 lambda 同理

```cpp
std::vector<int> v{2, 4, 6};
auto f = [&v](const auto& x) { v = x; };
f({1, 2, 3});  // 错误
```

* 如果返回的表达式递归调用函数，则不会发生推断

```cpp
auto f(int n) {
  if (n <= 1) {
    return 1;  // OK：返回类型被推断为 int
  } else {
    return n * f(n - 1);  // OK：f(n - 1) 为 int，所以 n * f(n - 1) 也为 int
  }
}

auto g(int n) {
  if (n > 1) {
    return n * g(n - 1);  // 错误：g(n - 1) 类型未知
  } else {
    return 1;
  }
}
```

* 没有返回值时 auto 返回类型会推断为 void，若不能匹配 void 则出错

```cpp
auto f1() {}           // OK：返回类型是 void
auto f2() { return; }  // OK：返回类型是 void
auto* f3() {}          // 错误：auto*不能推断为 void
```

### C++17 的 auto

* C++17 中，auto 可以作为非类型模板参数

```cpp
namespace jc {

template <auto N, typename T = decltype(N)>
constexpr T add(T n) {
  return n + N;
}

template <typename T, T N = T{}>
constexpr T add(T n) {
  return n + N;
}

}  // namespace jc

static_assert(jc::add<2>(3) == 5);
static_assert(jc::add<int>(3) == 3);

int main() {}
```

* C++17 引入了[结构化绑定（structured binding）](https://en.cppreference.com/w/cpp/language/structured_binding)

```cpp
#include <cassert>
#include <string>
#include <tuple>

namespace jc {

struct A {
  int n = 42;
  std::string s = "hello";
};

A f() { return {}; }

}  // namespace jc

int main() {
  const auto&& [n, s] = jc::f();
  assert(n == 42);
  assert(s == "hello");

  int a[] = {1, 2, 3};
  auto& [x, y, z] = a;
  assert(&x == a);
  assert(&y == a + 1);
  assert(&z == a + 2);

  auto t = std::make_tuple(true, 'c');
  auto& [b, c] = t;  // auto& b = std::get<0>(t); auto& c = std::get<1>(t);
  assert(&b == &std::get<0>(t));
  assert(&c == &std::get<1>(t));
}
```

* 特化 [std::tuple_size](https://en.cppreference.com/w/cpp/utility/tuple/tuple_size)、[std::tuple_element](https://en.cppreference.com/w/cpp/utility/tuple/tuple_element)、[std::get(std::tuple)](https://en.cppreference.com/w/cpp/utility/tuple/get) 即可生成一个 tuple-like 类

```cpp
#include <cassert>
#include <string>
#include <tuple>

namespace jc {

struct A {};

}  // namespace jc

namespace std {

template <>
struct std::tuple_size<jc::A> {
  static constexpr int value = 2;
};

template <>
struct std::tuple_element<0, jc::A> {
  using type = int;
};

template <>
struct std::tuple_element<1, jc::A> {
  using type = std::string;
};

template <int>
auto get(jc::A);

template <>
auto get<0>(jc::A) {
  return 42;
}

template <>
auto get<1>(jc::A) {
  return "hello";
}

}  // namespace std

int main() {
  auto&& [x, y] = jc::A{};
  static_assert(std::is_same_v<decltype(x), int>);
  static_assert(std::is_same_v<decltype(y), std::string>);
  assert(x == 42);
  assert(y == "hello");
}
```

## 03 [decltype](https://en.cppreference.com/w/cpp/language/decltype)

* decltype 会推断出直觉预期的类型

```cpp
const int i = 0;  // decltype(i) 为 const int

struct Point {
  int x, y;  // decltype(Point::x) 和 decltype(Point::y) 为 int
};

A a;                 // decltype(a) 为 A
bool f(const A& x);  // decltype(x) 为 const A&，decltype(f) 为 bool(const A&)
if (f(a)) {          // decltype(f(a)) 为 bool
}

int a[]{1, 2, 3};  // decltype(a) 为 int[3]
```

* decltype 一般用来声明与参数类型相关的返回类型。比如下面模板的参数是容器和索引，而返回类型取决于元素类型

```cpp
template <typename Container, typename Index>
auto f(Container& c, Index i) -> decltype(c[i]) {
  return c[i];  // auto 只表示使用类型推断，推断的是 decltype
}
```

* C++14 允许省略尾置返回类型，只留下 auto

```cpp
template <typename Container, typename Index>
auto f(Container& c, Index i) {
  return c[i];
}
```

* 但直接使用会发现问题

```cpp
std::vector<int> v{1, 2, 3};
f(v, 1) = 42;  // 返回 v[1] 然后赋值为 42，但不能通过编译
```

* operator[] 返回元素引用，类型为 int&，但 auto 推断为 int，因此上面的操作相当于给一个整型值赋值，显然是错误的
* 为了得到期望的返回类型，需要对返回类型使用 decltype 的推断机制，C++14 允许将返回类型声明为 decltype(auto) 来实现这点

```cpp
template <typename Container, typename Index>
decltype(auto) f(Container& c, Index i) {
  return c[i];
}
```

* decltype(auto) 也可以作为变量声明类型

```cpp
int i = 1;
const int& j = i;
decltype(auto) x = j;  // const int& x = j;
```

* 但还有一些问题，容器传的是 non-const 左值引用，这就无法接受右值

```cpp
std::vector<int> make_v();  // 工厂函数
auto i = f(make_v(), 5);
```

* 为了同时匹配左值和右值而又不想重载，只需要模板参数写为转发引用

```cpp
template <typename Container, typename Index>
decltype(auto) f(Container&& c, Index i) {
  return std::forward<Container>(c)[i];  // 传入的实参是右值时，将 c 转为右值
}

// C++11 版本
template <typename Container, typename Index>
auto f(Container&& c, Index i) -> decltype(std::forward<Container>(c)[i]) {
  authenticate_user();
  return std::forward<Container>(c)[i];
}
```

### decltype 的特殊情况

* 如果表达式是解引用，decltype 会推断为引用类型

```cpp
int* p;  // decltype(*p) 是 int&
```

* 赋值表达式会产生引用，类型为赋值表达式中左值的引用类型

```cpp
int a = 0;
int b = 1;
decltype(a = 1) c = b;  // int&
c = 3;
std::cout << a << b << c;  // 033
```

* 如果表达式加上一层或多层括号，编译器会将其看作表达式，变量是一种可以作为赋值语句左值的特殊表达式，因此也得到引用类型。decltype((variable)) 结果永远是引用，declytpe(variable) 只有当变量本身是引用时才是引用

```cpp
int i;  // decltype((i)) 是 int&
```

* 在返回类型为 decltype(auto) 时，这可能导致返回局部变量的引用

```cpp
decltype(auto) f1() {
  int x = 0;
  return x;  // decltype(x) 是 int，因此返回 int
}

decltype(auto) f2() {
  int x = 0;
  return (x);  // decltype((x)) 是 int&，因此返回了局部变量的引用
}
```

## 04 查看推断类型的方法

* 实际开发中常用的方法是在 IDE 中将鼠标停放在变量上，现代 IDE 通常会显示出推断的类型
* 利用报错信息，比如写一个声明但不定义的类模板，用这个模板创建实例时将出错，编译将提示错误原因

```cpp
template <typename T>
class A;

A<decltype(x)> xType;  // 未定义类模板，错误信息将提示 x 类型
// 比如对 int x 报错如下
// error C2079 : “xType” 使用未定义的 class“A<int>”
```

* 使用 [type_id](https://en.cppreference.com/w/cpp/language/typeid) 和 [std::type_info::name](https://en.cppreference.com/w/cpp/types/type_info/name) 获取类型，但得到的类型会忽略 cv 和引用限定符

```cpp
template <typename T>
void f(T& x) {
  std::cout << "T = " << typeid(T).name() << '\n';
  std::cout << "x = " << typeid(x).name() << '\n';
}
```

* 使用 [Boost.TypeIndex](https://www.boost.org/doc/libs/1_79_0/doc/html/boost_typeindex_header_reference.html#header.boost.type_index_hpp) 可以得到精确类型

```cpp
#include <boost/type_index.hpp>
#include <iostream>

template <typename T>
void f(const T& x) {
  using boost::typeindex::type_id_with_cvr;
  std::cout << "T = " << type_id_with_cvr<T>().pretty_name() << '\n';
  std::cout << "x = " << type_id_with_cvr<decltype(x)>().pretty_name() << '\n';
}
```
