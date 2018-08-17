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

#include "math/ndarray.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>

namespace fetch {
namespace math {

template <typename T>
void BuildNDArray(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<NDArray<T>, fetch::math::ShapeLessArray<T>>(module, custom_name.c_str())
      .def(py::init<std::size_t const &>())
      .def("copy", [](NDArray<T> &a) { return a.Copy(); })
      .def("copy",
           [](NDArray<T> &a, NDArray<T> &b) {
             a.Copy(b);
             return a;
           })
      .def("L2Loss", [](NDArray<T> &a) { return a.L2Loss(); })
      .def("Flatten",
           [](NDArray<T> &a) {
             a.Flatten();
             return a;
           })
      .def("__eq__",
           [](NDArray<T> &s, NDArray<T> const &other) {
             if (other.size() != s.size()) throw py::index_error();
             s.Copy(other);
           })
      .def("__add__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Divide(b, c);
             return a;
           })
      .def("__add__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a;
             a.LazyReshape(b.shape());
             a.Divide(b, c);
             return a;
           })
      .def("__iadd__",
           [](NDArray<T> &a, NDArray<T> const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](NDArray<T> &a, NDArray<T> const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](NDArray<T> &a, NDArray<T> const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](NDArray<T> &a, NDArray<T> const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__iadd__",
           [](NDArray<T> &a, T const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](NDArray<T> &a, T const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](NDArray<T> &a, T const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](NDArray<T> &a, T const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("reshape",
           [](NDArray<T> &a, std::vector<std::size_t> b) {
             a.CanReshape(b);
             a.Reshape(b);
             return;
           })
      .def("Max", [](NDArray<T> &a) { return a.Max(); })
      //      .def("min",
      //           [](NDArray<T> &a) { return a.Min(); })
      //      .def("reduce_sum",
      //           [](NDArray<T> &a) { return a.Sum(); })
      //      .def("reduce_product",
      //           [](NDArray<T> &a) { return a.Product(); })
      .def("shape", [](NDArray<T> &a) { return a.shape(); });

  //             .def("Relu",
  //                  [](NDArray<T> &a, NDArray<T> &b)
  //                  {
  //                    a.Relu(b);
  //                    return a;
  //                  });
}

}  // namespace math
}  // namespace fetch
