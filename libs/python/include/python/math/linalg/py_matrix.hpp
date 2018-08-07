#pragma once

#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace linalg {

template <typename T>
void BuildMatrix(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Matrix<T>, fetch::math::RectangularArray<T>>(module, custom_name.c_str())
      .def(py::init<>())
      .def(py::init<const Matrix<T> &>())
      .def(py::init<const typename fetch::math::linalg::Matrix<T>::super_type &>())
      .def(py::init<const std::size_t &, const std::size_t &>())
      .def(py::init<const fetch::byte_array::ByteArray>())
      .def(py::init<const std::string>())

      .def_static("Zeros", &Matrix<T>::Zeros)
      .def_static("UniformRandom", &Matrix<T>::UniformRandom)

      .def("Copy",
           [](Matrix<T> const &other) {
             Matrix<T> ret;
             ret.LazyResize(other.height(), other.width());
             ret.Copy(other);

             return ret;
           })

      .def("__add__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Divide(b, c);
             return a;
           })
      .def("__add__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             a.Divide(b, c);
             return a;
           })
      .def("__iadd__",
           [](Matrix<T> &a, Matrix<T> const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](Matrix<T> &a, Matrix<T> const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](Matrix<T> &a, Matrix<T> const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](Matrix<T> &a, Matrix<T> const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__iadd__",
           [](Matrix<T> &a, T const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](Matrix<T> &a, T const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](Matrix<T> &a, T const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](Matrix<T> &a, T const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__len__", [](Matrix<T> const &a) { return a.size(); })
      //    .def("Invert", &Matrix< T >::Invert)
      // Matrix-matrix ions
      .def("Transpose", (Matrix<T> & (Matrix<T>::*)(Matrix<T> const &)) & Matrix<T>::Transpose)
//      .def("Sum", &Matrix<T>::Sum)
      .def("Dot", [](Matrix<T> &a, Matrix<T> const &b, Matrix<T> const &c) {
        if (b.width() != c.height())
        {
          throw pybind11::index_error("matrix size mismatch");
        }

        a.Resize(b.height(), c.width());
        a.Dot(b, c);
      });
}
};  // namespace linalg
};  // namespace math
};  // namespace fetch
