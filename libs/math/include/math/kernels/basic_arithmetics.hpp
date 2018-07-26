#pragma once

namespace fetch {
namespace kernels {
namespace basic_aritmetics {

template <typename vector_register_type>
struct Add
{
  void operator()(vector_register_type const &x, vector_register_type const &y,
                  vector_register_type &z) const
  {
    z = x + y;
  }
};

}  // namespace basic_aritmetics
}  // namespace kernels
}  // namespace fetch

