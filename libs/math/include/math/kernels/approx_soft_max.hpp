#pragma once

namespace fetch {
namespace kernels {

template <typename type, typename vector_register_type>
struct ApproxSoftMax
{

  template <typename array_type>
  void operator()(array_type &B, array_type const &A) const
  {
    sum_ = vector_register_type(type(0));
    B.in_parallel().Apply(*this, &ApproxSoftMax::ExponentiateAndSum, A);
    scale_ = type(1. / reduce(sum_));
    B.in_parallel().Apply(*this, &ApproxSoftMax::ScaleElements, B);
  }

  void ExponentiateAndSum(vector_register_type const &a,
                          vector_register_type &      b) const
  {
    vector_register_type e = approx_exp(a);
    sum_                   = sum_ + e;
    b                      = e;
  };

  void ScaleElements(vector_register_type const &a,
                     vector_register_type &      b) const
  {
    b = a * scale_;
  };

private:
  vector_register_type sum_;
  vector_register_type scale_;
};

}  // namespace kernels
}  // namespace fetch

