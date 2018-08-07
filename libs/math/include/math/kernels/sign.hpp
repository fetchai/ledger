#pragma once

namespace fetch {
namespace kernels {

template <typename vector_register_type>
struct Sign
{
  void operator()(vector_register_type const &x, vector_register_type &y) const
  {
    const vector_register_type zero(typename vector_register_type::type(0));
    const vector_register_type one(typename vector_register_type::type(1));
    const vector_register_type min_one(typename vector_register_type::type(-1));

    y = ((x == zero) * (zero)) + ((x > zero) * (one)) + ((x < zero) * (min_one));
  }
};

}  // namespace kernels
}  // namespace fetch
