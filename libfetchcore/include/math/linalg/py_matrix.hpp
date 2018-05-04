#ifndef LIBFETCHCORE_MATH_LINALG_MATRIX_HPP
#define LIBFETCHCORE_MATH_LINALG_MATRIX_HPP
#include "math/linalg/matrix.hpp"

#include"fetch_pybind.hpp"
namespace fetch
{
namespace math
{
namespace linalg
{

template< typename T >
void BuildMatrix(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<Matrix< T >, fetch::memory::RectangularArray<T>>(module, custom_name.c_str() )
    .def(py::init<  >())
    .def(py::init< const Matrix<T> & >())    
    .def(py::init< const typename fetch::math::linalg::Matrix<T>::super_type & >())
    .def(py::init< const std::size_t &, const std::size_t & >())
    .def("Copy", &Matrix< T >::Copy) // TODO: Replace with lambda
    .def("Min", &Matrix< T >::Min)
    .def("Invert", &Matrix< T >::Invert)

    // Matrix-matrix operations
    .def("__add__",[](Matrix< T > const &a, Matrix< T > const &b) {
        return a + b;
      })
    .def("__sub__",[](Matrix< T > const &a, Matrix< T > const &b) {
        return a - b;
      })
    .def("__mul__",[](Matrix< T > const &a, Matrix< T > const &b) {
        return a * b;
      })     
    .def("__div__",[](Matrix< T > const &a, Matrix< T > const &b) {
        return a / b;
      }) 
    .def("__iadd__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator+=)
    .def("__isub__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator-=)
    .def("__imul__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator*=)
    .def("__idiv__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator/=)

    // Matrix-scalar operations
    .def("__add__",[](Matrix< T > const &a, double const &b) {
        return a + b;
      })
    .def("__sub__",[](Matrix< T > const &a, double const &b) {
        return a - b;
      })
    .def("__mul__",[](Matrix< T > const &a, double const &b) {
        return a * b;
      })     
    .def("__div__",[](Matrix< T > const &a, double const &b) {
        return a / b;
      }) 
    .def("__iadd__",[](Matrix< T > &a, double const &b) {
        a += b;
      }) 
    .def("__isub__",  [](Matrix< T > &a, double const &b) {
        a -= b;
      }) 
    .def("__imul__",  [](Matrix< T > &a, double const &b) {
        a *= b;
      }) 
    .def("__idiv__",  [](Matrix< T > &a, double const &b) {
        a /= b;
      }) 
    .def("__add__",[](Matrix< T > const &a, int const &b) {
        return a + b;
      })
    .def("__sub__",[](Matrix< T > const &a, int const &b) {
        return a - b;
      })
    .def("__mul__",[](Matrix< T > const &a, int const &b) {
        return a * b;
      })     
    .def("__div__",[](Matrix< T > const &a, int const &b) {
        return a / b;
      }) 
    .def("__iadd__",[](Matrix< T > &a, int const &b) {
        a += b;
      }) 
    .def("__isub__",  [](Matrix< T > &a, int const &b) {
        a -= b;
      }) 
    .def("__imul__",  [](Matrix< T > &a, int const &b) {
        a *= b;
      }) 
    .def("__idiv__",  [](Matrix< T > &a, int const &b) {
        a /= b;
      }) 

    
    .def("Max", &Matrix< T >::Max)
    .def("Transpose", &Matrix< T >::Transpose)
    .def("Sum", &Matrix< T >::Sum)
    .def("DotReference", &Matrix< T >::DotReference)
    .def("Dot", &Matrix< T >::Dot)
    .def("Mean", &Matrix< T >::Mean)
    .def("DotTransposedOf", &Matrix< T >::DotTransposedOf)
    .def("AbsMin", &Matrix< T >::AbsMin)
    .def("AbsMax", &Matrix< T >::AbsMax);

}
};
};
};

#endif
