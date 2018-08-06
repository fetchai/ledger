#pragma once

namespace fetch {
namespace kernels {

template <typename vector_register_type>
struct SumOfSquares
{
  void operator()(vector_register_type const &x, vector_register_type &y) const
  {
    vector_register_type e = x * x;
    sum_                   = sum_ + e;
    y                      = sum_;
  }

private:
  vector_register_type sum_ = (typename vector_register_type::type(0));
};

}  // namespace kernels
}  // namespace fetch
