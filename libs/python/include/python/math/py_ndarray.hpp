#pragma once

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
      .def(py::init<std::vector<std::size_t> const &>())
      .def("Copy", [](NDArray<T> &a) { return a.Copy(); })
      .def("Copy",
           [](NDArray<T> &a, NDArray<T> &b) {
             assert(a.size() = b.size());
             assert(a.shape() = b.shape());
             a.Copy(b);
             return a;
           })
      .def("Flatten",
           [](NDArray<T> &a) {
             a.Flatten();
             return a;
           })
      .def("__add__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](NDArray<T> const &b, NDArray<T> const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Divide(b, c);
             return a;
           })
      .def("__add__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](NDArray<T> const &b, T const &c) {
             NDArray<T> a(b.size());
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
      .def("__getitem__", [](NDArray<T> const &s, std::vector<std::tuple<std::size_t, std::size_t>> const &idxs)
      {
        NDArrayView arr_view;
        for (auto item : idxs)
        {
          arr_view.from.push_back(std::get<0>(item));
          arr_view.to.push_back(std::get<1>(item));
          arr_view.step.push_back(1);
        }
        std::cout << "HI THERE!" << std::endl;
        std::cout << arr_view.from[0] << std::endl;
        std::cout << s(arr_view)[0] << std::endl;
        std::cout << s(arr_view)[1] << std::endl;
        std::cout << s(arr_view)[2] << std::endl;
        std::cout << s(arr_view)[3] << std::endl;

        return s(arr_view);

      })
//      {
//        NDArray<T> a(s.size());
//        return s.template Get<NDArray<T>>(a, args...);
//      })
      //      .def("__getitem__",
      //        [](const NDArray<T> &s, Args ... args)
      //        {
      //          NDArray<T> a(s.size());
      //          return s.template Get<NDArray<T>>(a, args...);
      //        })
      //      .def("__setitem__",
      //           [](NDArray<T> &s, std::size_t i, T const &v) {
      //             if (i >= s.size()) throw py::index_error();
      //             s[i] = v;
      //           })
      .def("Reshape",
           [](NDArray<T> &a, std::vector<std::size_t> b) {
             a.Reshape(b);
             return;
           })
      .def("Shape", [](NDArray<T> &a) { return a.shape(); });
  //             .def("Relu",
  //                  [](NDArray<T> &a, NDArray<T> &b)
  //                  {
  //                    a.Relu(b);
  //                    return a;
  //                  });
}
};  // namespace math
};  // namespace fetch