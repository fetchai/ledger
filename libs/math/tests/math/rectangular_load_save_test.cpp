#include <iostream>
#include "core/random/lfg.hpp"
#include "math/rectangular_array.hpp"

using namespace fetch::math;
static fetch::random::LinearCongruentialGenerator gen;

int main() {
  RectangularArray<uint64_t> a, b;
  a.Resize(3, 3);
  for (auto &v : a) {
    v = gen();
  }
  std::cout << "Saving " << std::endl;

  a.Save("test.array");
  std::cout << "Loading" << std::endl;
  b.Load("test.array");
  std::cout << "Ready" << std::endl;
  if (a.size() != b.size()) {
    std::cout << "Failed 1: " << a.size() << " " << b.size() << std::endl;
    return -1;
  }
  std::cout << "Checking " << std::endl;

  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      std::cout << "Failed 2! " << i << " " << a[i] << " " << b[i] << std::endl;
      return -1;
    }
  }
  return 0;
}
