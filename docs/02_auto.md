## 05 用 [auto](https://en.cppreference.com/w/cpp/language/auto) 替代显式类型声明

* auto 声明的变量必须初始化，因此使用 auto 可以避免忘记初始化的问题

```cpp
int a;   // 潜在的未初始化风险
auto b;  // 错误：必须初始化
```

* 对于名称非常长的类型，如迭代器相关的类型，用 auto 声明可以大大简化工作

```cpp
template <typename It>
void f(It first, It last) {
  while (first != last) {
    auto val = *first;
    // auto 相当于 typename std::iterator_traits<It>::value_type
  }
}
```

* lambda 生成的闭包类型是编译期内部的匿名类型，无法得知，使用 auto 推断就没有这个问题

```cpp
auto f = [](auto& x, auto& y) { return x < y; };
```

* 如果不使用 auto，可以改用 [std::function](https://en.cppreference.com/w/cpp/utility/functional/function)

```cpp
// std::function 的模板参数中不能使用 auto
std::function<bool(int&, int&)> f = [](auto& x, auto& y) { return x < y; };
```

* 除了明显的语法冗长和不能利用 auto 参数的缺点，[std::function](https://en.cppreference.com/w/cpp/utility/functional/function) 与 auto 的最大区别在于，auto 和闭包类型一致，内存量和闭包相同，而 [std::function](https://en.cppreference.com/w/cpp/utility/functional/function) 是类模板，它的实例有一个固定大小，这个大小不一定能容纳闭包，于是会分配堆上的内存以存储闭包，导致比 auto 变量占用更多内存。此外，编译器一般会限制内联，[std::function](https://en.cppreference.com/w/cpp/utility/functional/function) 调用闭包会比 auto 慢
* auto 可以避免简写类型存在的潜在问题。比如如下代码有潜在隐患

```cpp
std::vector<int> v;
unsigned sz = v.size();  // v.size() 类型实际为 std::vector<int>::size_type
// 在 32 位机器上 std::vector<int>::size_type 与 unsigned 尺寸相同
// 但在 64 位机器上，std::vector<int>::size_type 是 64 位，而 unsigned 是 32 位

std::unordered_map<std::string, int> m;
for (const std::pair<std::string, int>& p : m) {
  // m 元素类型为 std::pair<const std::string, int>
  // 循环中使用的元素类型不一致，需要转换，期间将构造大量临时对象
}
```

* 如果显式类型声明能让代码更清晰或有其他好处就不用强行 auto，此外 IDE 的类型提示也能缓解不能直接看出对象类型的问题

## 06 [auto](https://en.cppreference.com/w/cpp/language/auto) 推断出非预期类型时，先强制转换出预期类型

* auto 推断得到的类型可能与直觉认知不同

```cpp
std::vector<bool> v{true, false};
for (auto& x : v) {  // 错误：未定义行为
}
```

* [std::vector\<bool\>](https://en.cppreference.com/w/cpp/container/vector_bool 不是真正的 STL 容器，也不包含 bool 类型元素。它是 [std::vector](https://en.cppreference.com/w/cpp/container/vector) 对于 bool 类型的特化，为了节省空间，每个元素用一个 bit（而非一个 bool，一个 bool 占一字节）表示，于是 [operator[]](https://en.cppreference.com/w/cpp/container/vector/operator_at) 返回的应该是单个 bit 的引用，但 C++ 中不存在指向单个 bit 的指针，因此也不能获取单个 bit 的引用

```cpp
std::vector<bool> v{true, false};
bool& p = v[0];  // 错误
```

* 因此需要一个行为类似单个 bit 并可以被引用的对象，也就是 [std::vector\<bool\>::reference](https://en.cppreference.com/w/cpp/container/vector_bool/reference)，它可以隐式转换为 bool

```cpp
bool x = v[0];
```

* 而 auto 推断不会进行隐式转换

```cpp
auto x = v[0];  // x 类型为 std::vector<bool>::reference
// x 不一定指向 std::vector<bool>的第 0 个 bit，
// 这取决于 std::vector<bool>::reference 的实现，
// 一种实现是含有一个指向一个 machine word的指针，
// word 持有被引用的 bit 和这个 bit 相对 word 的 offset，
// 于是 x 持有一个由 opeartor[] 返回的
// 临时的 machine word 的指针和 bit 的 offset，
// 这条语句结束后临时对象被析构，
// 于是 x 含有一个空悬指针，导致后续的未定义行为
```

* [std::vector\<bool\>::reference](https://en.cppreference.com/w/cpp/container/vector_bool/reference) 是一个代理类（proxy class，模拟或扩展其他类型的类）的例子，比如 [std::shared_ptr](https://en.cppreference.com/w/cpp/memory/shared_ptr) 和 [std::unique_ptr](https://en.cppreference.com/w/cpp/memory/unique_ptr) 是很明显的代理类。还有一些为了提高数值计算效率而使用表达式模板技术开发的类，比如给定一个 Matrix 类和它的对象

```cpp
Matrix sum = m1 + m2 + m3 + m4;
```

* Matrix 对象的 operator+ 返回的是结果的代理而非结果本身，这样可以使得表达式的计算更为高效

```cpp
auto x = m1 + m2;  // x 可能是 Sum<Matrix, Matrix> 而不是 Matrix 对象
```

* auto 推断出代理类的问题实际很容易解决，事先做一次到预期类型的强制转换即可

```cpp
auto x = static_cast<bool>(v[0]);
```
