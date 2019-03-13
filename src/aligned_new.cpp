#include <cassert>
#include <cstddef>
#include <new>
#include <string>

namespace jc {

template <typename T, std::size_t _Element_Count = 1, bool _Default_Ctor = true>
T* aligned_new() {
  if constexpr (_Element_Count == 0) {
    return nullptr;
  }
  T* res = reinterpret_cast<T*>(::operator new (
      sizeof(T) * _Element_Count,
      std::align_val_t{std::hardware_constructive_interference_size}));
  if constexpr (_Default_Ctor) {
    new (res) T[_Element_Count];
  }
  return res;
}

template <typename T, std::size_t _Element_Count = 1, bool _Dtor = true>
void aligned_release(T* p) {
  if constexpr (_Element_Count == 0) {
    return;
  }
  if (!p) {
    return;
  }
  if constexpr (_Dtor) {
    for (std::size_t i = 0; i < _Element_Count; ++i) {
      p[i].~T();
    }
  }
  ::operator delete (
      p, sizeof(T) * _Element_Count,
      std::align_val_t{std::hardware_constructive_interference_size});
}

}  // namespace jc

int main() {
  constexpr std::size_t n = 3;
  std::string* p = jc::aligned_new<std::string, n>();
  for (std::size_t i = 0; i < n; ++i) {
    p[i] = std::to_string(i);
  }
  for (std::size_t i = 0; i < n; ++i) {
    assert(p[i] == std::to_string(i));
  }
  jc::aligned_release<std::string, n>(p);
}
