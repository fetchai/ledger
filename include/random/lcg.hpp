#ifndef MATH_RANDOM_LCG_HPP
#define MATH_RANDOM_LCG_HPP
#include <cstdlib>
#include <limits>

namespace fetch {
namespace random {
class LinearCongruentialGenerator {
 public:
  typedef uint64_t random_type;

  random_type Seed() const { return seed_; }
  random_type Seed(random_type const &s) { return x_ = seed_ = s; }
  void Reset() { Seed(Seed()); }
  random_type operator()() { return x_ = x_ * a_ + c_; }
  double AsDouble() { return double(this->operator()()) * inv_double_max_; }

  static constexpr random_type max() {
    return std::numeric_limits<random_type>::max();
  }

  static constexpr random_type min() {
    return std::numeric_limits<random_type>::min();
  }

 private:
  random_type x_ = 1;
  random_type seed_ = 1;
  random_type a_ = 6364136223846793005ull;
  random_type c_ = 1442695040888963407ull;

  static constexpr double inv_double_max_ =
    1. / double(std::numeric_limits<random_type>::max());
};
}
}
#endif
