#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "math/free_functions/free_functions.hpp"
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
      .def_static("Zeroes",
                  (Matrix<T>(*)(std::size_t const &, std::size_t const &)) & Matrix<T>::Zeroes)
      .def_static("Zeroes", (Matrix<T>(*)(std::vector<std::size_t> const &)) & Matrix<T>::Zeroes)
      .def_static("UniformRandom", (Matrix<T>(*)(std::size_t const &, std::size_t const &)) &
                                       Matrix<T>::UniformRandom)
      .def_static("UniformRandom",
                  (Matrix<T>(*)(std::vector<std::size_t> const &)) & Matrix<T>::UniformRandom)
      .def("Shape", [](Matrix<T> const &obj) { return obj.shape(); })
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
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Subtract(b, c, a);
             return a;
           })
      .def("__div__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Divide(b, c, a);
             return a;
           })
      .def("__add__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Subtract(b, c, a);
             return a;
           })
      .def("__rsub__",
           [](Matrix<T> &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Subtract(c, b, a);
             return a;
           })
      .def("__div__",
           [](Matrix<T> const &b, T const &c) {
             Matrix<T> a;
             a.LazyResize(b.height(), b.width());
             Divide(b, c, a);
             return a;
           })
      .def("__div__",
           [](T const &b, Matrix<T> const &c) {
             Matrix<T> a;
             a.LazyResize(c.height(), c.width());
             Divide(b, c, a);
             return a;
           })
      .def("__iadd__",
           [](Matrix<T> &a, Matrix<T> const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](Matrix<T> &a, Matrix<T> const &b) {
             Matrix<T> c{a.height(), a.width()};
             fetch::math::Multiply(a, b, c);
             return c;
           })
      .def("__isub__",
           [](Matrix<T> &a, Matrix<T> const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__itruediv__",
           [](Matrix<T> &a, Matrix<T> const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__itruediv__",
           [](Matrix<T> &a, T const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__rtruediv__",
           [](Matrix<T> &c, T const &a) {
             Matrix<T> ret(c.height(), c.width());
             Divide(a, c, ret);
             return ret;
           })
      .def("__iadd__",
           [](Matrix<T> &a, T const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](Matrix<T> &a, T const &b) {
             Matrix<T> c{a.height(), a.width()};
             fetch::math::Multiply(a, b, c);
             return c;
           })
      .def("__isub__",
           [](Matrix<T> &a, T const &c) {
             a.InlineSubtract(c);
             return a;
           })

      .def("__len__", [](Matrix<T> const &a) { return a.size(); })

      .def("__ge__",
           [](Matrix<T> const &b, Matrix<T> const &c) {
             Matrix<T> a{b.height(), b.width()};
             Isgreaterequal(b, c, a);
             return a;
           })

      .def("Maximum",
           [](Matrix<T> &a, Matrix<T> const &b) {
             if ((a.height() != b.height()) or (a.width() != b.width()))
             {
               throw pybind11::index_error("matrix size mismatch");
             }
             Matrix<T> ret(a.height(), b.width());

             Maximum(a, b, ret);
             return ret;
           })
      .def("ArgMax",
           [](Matrix<T> &a, std::size_t const axis) {
             if (axis >= 2)
             {
               throw py::index_error();
             }

             std::size_t ret_len;
             if (axis == 0)
             {
               ret_len = a.width();
             }
             else
             {
               ret_len = a.height();
             }
             ShapeLessArray<T> ret{ret_len};
             ArgMax(a, axis, ret);
             return ret;
           })
      //    .def("Invert", &Matrix< T >::Invert)
      // Matrix-matrix ions
      .def("Transpose", (Matrix<T> & (Matrix<T>::*)(Matrix<T> const &)) & Matrix<T>::Transpose)
      //      .def("Sum", &Matrix<T>::Sum)
      .def("DotTranspose",
           [](Matrix<T> &a, Matrix<T> const &b, Matrix<T> const &c) {
             if (b.width() != c.width())
             {
               throw pybind11::index_error("matrix size mismatch");
             }
             return a.DotTranspose(b, c);
           })
      .def("TransposeDot",
           [](Matrix<T> &a, Matrix<T> const &b, Matrix<T> const &c) {
             if (b.height() != c.height())
             {
               throw pybind11::index_error("matrix size mismatch");
             }

             return a.TransposeDot(b, c);
           })
      .def("Dot", [](Matrix<T> &a, Matrix<T> const &b, Matrix<T> const &c) {
        if (b.width() != c.height())
        {
          throw pybind11::index_error("matrix size mismatch");
        }

        //        a.Resize(b.height(), c.width());

        return a.Dot(b, c);
      });
}

}  // namespace linalg
}  // namespace math
}  // namespace fetch
