#pragma once

namespace fetch {
namespace kernels {

template <typename type, typename vector_register_type>
struct L2Norm
{
  void operator()(vector_register_type &y, vector_register_type const &x) const
  {
    y = this->Reduction(x);
    y = y / 2;
  }

  type Reduction(vector_register_type const &A) const
  {
    type ret = 0;
    ret = A.in_parallel().Reduce([](vector_register_type const &a,
                                    vector_register_type const &b)
    {
      return a * a;
    });

    return ret;
  }

private:
//  vector_register_type sum_ = (typename vector_register_type::type(0));
};

}  // namespace kernels
}  // namespace fetch




