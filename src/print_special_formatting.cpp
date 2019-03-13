#include <iomanip>
#include <iostream>

namespace jc {

class format_guard {
 public:
  ~format_guard() { std::cout.flags(f); }

 private:
  decltype(std::cout.flags()) f{std::cout.flags()};
};

template <typename T>
struct scientific_type {
  T value;
  explicit scientific_type(T val) : value{val} {}
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const scientific_type<T> &w) {
  format_guard _;
  return os << std::scientific << std::uppercase << std::showpos << w.value;
}

}  // namespace jc

int main() {
  {
    jc::format_guard _;
    std::cout << std::hex << std::scientific << std::showbase << std::uppercase;

    std::cout << "Numbers with special formatting:\n";
    std::cout << 0x123abc << '\n';
    std::cout << 0.123456789 << '\n';
  }
  std::cout << "Same numbers, but normal formatting again:\n";
  std::cout << 0x123abc << '\n';
  std::cout << 0.123456789 << '\n';
  std::cout << "Mixed formatting: " << 123.0 << " "
            << jc::scientific_type{123.0} << " " << 123.456 << '\n';
}
