#pragma once

#include "math/ndarray.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>

namespace fetch {
namespace math {

template<typename T>
void BuildNDArray(std::string const &custom_name, pybind11::module &module) {

  namespace py = pybind11;
  py::class_<NDArray<T>, fetch::math::ShapeLessArray<T>>(module, custom_name.c_str())
          .def(py::init<std::size_t const &>())
          .def("Copy",
               [](NDArray<T> &a)
               {
                 return a.Copy();
               })
          .def("Copy",
               [](NDArray<T> &a, NDArray<T> &b)
               {
                 a.Copy(b);
                 return a;
               })
          .def("Flatten",
               [](NDArray<T> &a)
               {
                 a.Flatten();
                 return a;
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

          .def("Reshape",
               [](NDArray<T> &a, std::vector<std::size_t> b)
               {
                 a.Reshape(b);
                 return;
               })
          .def("Shape",
               [](NDArray<T> &a)
               {
                 return a.shape();
               });
//             .def("Relu",
//                  [](NDArray<T> &a, NDArray<T> &b)
//                  {
//                    a.Relu(b);
//                    return a;
//                  });

}
};
};