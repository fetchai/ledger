#ifndef LIBFETCHCORE_MATH_LINALG_MATRIX_HPP
#define LIBFETCHCORE_MATH_LINALG_MATRIX_HPP
#include "math/linalg/matrix.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
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
//    .def(py::init< Matrix<T> && >())
    .def(py::init< const Matrix<T> & >())    
    .def(py::init< const typename fetch::math::linalg::Matrix<T>::super_type & >())
//    .def(py::init< typename fetch::math::linalg::Matrix<T>::super_type && >())
    .def(py::init< const std::size_t &, const std::size_t & >())
    //    .def(py::self = py::self )
    .def("Min", &Matrix< T >::Min)
    .def("Invert", &Matrix< T >::Invert)
    .def("__add__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator+)
    .def("__sub__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator-)
    .def("__mul__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator*)
    .def("__div__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator/)
    .def("__iadd__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator+=)
    .def("__isub__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator-=)
    .def("__imul__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator*=)
    .def("__idiv__",  (Matrix< T >& (Matrix< T >::*)(Matrix< T > const &)) &Matrix< T >::operator/=)

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
