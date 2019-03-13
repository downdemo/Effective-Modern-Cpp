#include <algorithm>
#include <cassert>
#include <execution>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <vector>

namespace jc {

template <std::size_t N, std::size_t Min, std::size_t Max>
std::vector<int> random_sequence() {
  std::random_device rd;
  std::mt19937 mt{rd()};
  std::uniform_int_distribution<int> d(Min, Max);
  auto rand_num([=]() mutable { return d(mt); });
  std::vector<int> res(N);
  std::generate(std::execution::par, res.begin(), res.end(), rand_num);
  return res;
}

}  // namespace jc

int main() {
  constexpr std::size_t N = 100000;
  constexpr std::size_t Min = 0;
  constexpr std::size_t Max = 10;
  auto v = jc::random_sequence<N, Min, Max>();
  std::sort(std::execution::par, v.begin(), v.end());
  std::size_t n = 0;
  std::vector<std::size_t> cnt;
  for (auto i = Min; i <= Max; ++i) {
    cnt.emplace_back(std::count_if(std::execution::par, v.begin(), v.end(),
                                   [i](int x) { return x == i; }));
  }
  assert(std::accumulate(cnt.begin(), cnt.end(), 0) == N);
  std::copy(cnt.begin(), cnt.end(),
            std::ostream_iterator<std::size_t>{std::cout, "\n"});
}
