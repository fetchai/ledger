#ifndef KERNELS_APPROX_LOG_HPP
#define KERNELS_APPROX_LOG_HPP

namespace fetch {
namespace kernels {

template <typename vector_register_type>
struct ApproxLog
{
  void operator()(vector_register_type const &x, vector_register_type &y) const
  {
    y = approx_log(x);
  }
};

}  // namespace kernels
}  // namespace fetch

#endif
