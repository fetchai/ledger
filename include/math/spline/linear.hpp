#ifndef MATH_SPLINE_LINEAR_HPP
#define MATH_SPLINE_LINEAR_HPP
#include <cassert>
#include <vector>
namespace fetch {
namespace math {
namespace spline {

template <typename T = double>
class Spline {
 public:
  typedef T type;

  template <typename F>
  void SetFunction(F &f, type from, type to, std::size_t n) {
    assert(n < 8 * sizeof(uint64_t));
    range_from_ = from;
    range_to_ = to;
    range_span_ = range_to_ - range_from_;

    data_.resize(1 << n);
    std::size_t m = size();
    range_to_index_ = (m - 1) / range_span_;

    type x = range_from_, delta = range_span_ / (m - 1);
    for (std::size_t i = 0; i < m; ++i) {
      data_[i] = f(x);
      x += delta;
    }
  }

  type operator()(type x) {
    double z = (x - range_from_) * range_to_index_;
    uint32_t i = z;
    z -= i;
    return (data_[i + 1] - data_[i]) * z + data_[i];
  }

  std::size_t size() const { return data_.size(); }

 private:
  double range_from_, range_to_, range_span_;
  double range_to_index_;
  double value_from_, value_to, value_span_;
  std::vector<double> data_;
};
};
};
};
#endif
