#ifndef KERNELS_SCALARS_HPP
#define KERNELS_SCALARS_HPP
#include <cmath>

namespace fetch {
namespace kernels {

template <typename type, typename vector_register_type>
struct MultiplyScalar
{
  MultiplyScalar(type const &val) : scalar(val) {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar * x;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct AddScalar
{
  AddScalar(type const &val) : scalar(val) {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar + x;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct SubtractScalar
{
  SubtractScalar(type const &val) : scalar(val) {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = x - scalar;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct DivideScalar
{
  DivideScalar(type const &val) : scalar(val) {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = x / scalar;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct ScalarSubtract
{
  ScalarSubtract(type const &val) : scalar(val) {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar - x;
  }

  vector_register_type scalar;
};

template <typename type, typename vector_register_type>
struct DivideScalar
{
  ScalarDivide(type const &val) : scalar(val) {}

  void operator()(vector_register_type const &x, vector_register_type &y)
  {
    y = scalar / x;
  }

  vector_register_type scalar;
};

}  // namespace kernels
}  // namespace fetch

#endif
