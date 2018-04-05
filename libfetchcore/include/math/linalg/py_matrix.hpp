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
  py::class_<Matrix< T >, fetch::memory::RectangularArray<T>>(module, custom_name )
    .def(py::init<  >())
    .def(py::init< Matrix<T> && >())
    .def(py::init< const Matrix<T> & >())
    .def(py::init< const fetch::math::linalg::Matrix::super_type & >())
    .def(py::init< fetch::math::linalg::Matrix::super_type && >())
    .def(py::init< const std::size_t &, const std::size_t & >())
    .def(py::self = py::self )
    .def("Min", &Matrix< T >::Min)
    .def("Invert", &Matrix< T >::Invert)
    .def(py::self + py::self )
    .def(py::self * py::self )
    .def(py::self - py::self )
    .def(py::self / py::self )
    .def(py::self & py::self )
    .def(py::self -= py::self )
    .def(py::self += py::self )
    .def("Max", &Matrix< T >::Max)
    .def(py::self /= py::self )
    .def("Transpose", &Matrix< T >::Transpose)
    .def("Sum", &Matrix< T >::Sum)
    .def("DotReference", &Matrix< T >::DotReference)
    .def(py::self | py::self )
    .def(py::self |= py::self )
    .def("Dot", &Matrix< T >::Dot)
    .def("Mean", &Matrix< T >::Mean)
    .def("DotTransposedOf", &Matrix< T >::DotTransposedOf)
    .def("AbsMin", &Matrix< T >::AbsMin)
    .def(py::self &= py::self )
    .def(py::self *= py::self )
    .def("AbsMax", &Matrix< T >::AbsMax);

}
};
};
};

#endif