#include <functional>

namespace jc {

template <typename F, typename... Fs>
constexpr auto concat(F f, Fs... fs) {
  if constexpr (sizeof...(fs) > 0) {
    return [=](auto... args) { return f(concat(fs...)(args...)); };
  } else {
    return f;
  }
}

}  // namespace jc

int main() {
  auto twice = [](int i) { return i * 2; };
  auto thrice = [](int i) { return i * 3; };
  auto combined =
      jc::concat(twice, thrice, std::plus<int>{});  // twice(thrice(plus))
  static_assert(combined(2, 3) == 30);              // 30 = 2 * 3 * (2 + 3)
}