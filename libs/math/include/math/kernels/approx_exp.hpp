#ifndef KERNELS_APPROX_EXP_HPP
#define KERNELS_APPROX_EXP_HPP

namespace fetch {
namespace kernels {

template <typename vector_register_type>
struct ApproxExp
{
  void operator()(vector_register_type const &x, vector_register_type &y) const
  {
    y = approx_exp(x);
  }
};

}  // namespace kernels
}  // namespace fetch

#endif
