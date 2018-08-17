#pragma once

#include "math/exp.hpp"
#include "math/log.hpp"
#include "math/ndarray.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>

namespace fetch {
namespace math {

template <typename T>
void BuildNDArray(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  //             .def("Relu",
  py::class_<NDArray<T>, fetch::math::ShapeLessArray<T>>(module, custom_name.c_str())
      .def(py::init<std::size_t const &>())
      .def(py::init<std::vector<std::size_t> const &>())
      .def("Copy", [](NDArray<T> &a) { return a.Copy(); })
      .def("Copy",
           [](NDArray<T> &a, NDArray<T> &b) {
             assert(a.size() == b.size());
             assert(a.shape() == b.shape());
             a.Copy(b);
             return a;
           })
      .def("Flatten",
           [](NDArray<T> &a) {
             a.Flatten();
             return a;
           })
      .def("__add__",
           [](NDArray<T> &b, NDArray<T> &c) {
             NDArray<T>               a;
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();
             a = NDArray<T>::Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> &b, NDArray<T> &c) {
             NDArray<T>               a;
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();
             a = NDArray<T>::Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> &b, NDArray<T> &c) {
             NDArray<T>               a;
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();
             a = NDArray<T>::Subtract(b, c);
             return a;
           })
      .def("__truediv__",
           [](NDArray<T> &b, NDArray<T> &c) {
             NDArray<T>               a;
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();
             a = NDArray<T>::Divide(b, c);
             return a;
           })
      .def("__add__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a = b.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a = b.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a = b.Subtract(b, c);
             return a;
           })
      .def("__truediv__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a = b.Divide(b, c);
             return a;
           })
      .def("__iadd__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape))) throw py::index_error();
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape))) throw py::index_error();
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape))) throw py::index_error();
             a.InlineSubtract(c);
             return a;
           })
      //      .def("__truediv__",
      //           [](NDArray<T> &a, NDArray<T> &c) {
      //             a.InlineDivide(c);
      //             return a;
      //           })
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
      //      .def("__truediv__",
      //           [](NDArray<T> &a, T const &c) {
      //             a.InlineDivide(c);
      //             return a;
      //           })
      .def("__eq__",
           [](NDArray<T> const &s, NDArray<T> const &other) { return s.operator==(other); })
      .def("__ne__",
           [](NDArray<T> const &s, NDArray<T> const &other) { return s.operator!=(other); })
      .def("__getitem__",
           [](NDArray<T> const &s, std::size_t idx) {
             if (idx >= s.size()) throw py::index_error();
             return s[idx];
           })
      .def("__getitem__",
           [](NDArray<T> const &s, std::vector<py::slice> slices) {
             std::vector<size_t> start, stop, step, slicelength;
             for (std::size_t i = 0; i < slices.size(); ++i)
             {
               start.push_back(0);
               stop.push_back(0);
               step.push_back(0);
               slicelength.push_back(0);
             }

             for (std::size_t i = 0; i < slices.size(); ++i)
             {
               if (!slices[i].compute(s.shape()[i], &start[i], &stop[i], &step[i], &slicelength[i]))
                 throw py::error_already_set();
             }

             // set up the view to extract
             NDArrayView arr_view = NDArrayView();
             for (std::size_t i = 0; i < start.size(); ++i)
             {
               arr_view.from.push_back(start[i]);
               arr_view.to.push_back(stop[i]);
               arr_view.step.push_back(step[i]);
             }

             return s.GetRange(arr_view);
           })
      .def("__getitem__",
           [](NDArray<T> const &s, std::vector<std::vector<std::size_t>> const &idxs) {
             assert(idxs.size() > 0);
             for (auto cur_idx : idxs)
             {
               assert(cur_idx.size() == 3);
             }

             // set up the view to extract
             NDArrayView arr_view = NDArrayView();
             for (auto item : idxs)
             {
               arr_view.from.push_back(item[0]);
               arr_view.to.push_back(item[1]);
               arr_view.step.push_back(item[2]);
             }

             return s.GetRange(arr_view);
           })
      .def("__getitem__",
           [](NDArray<T> const &s, std::vector<std::size_t> const &idxs) {
             assert(idxs.size() == s.shape().size());

             return s.Get(idxs);
           })
      .def("__setitem__",
           [](NDArray<T> &s, std::vector<py::slice> slices, NDArray<T> const &t) {
             // manage slice assignment
             std::vector<size_t> start, stop, step, slicelength;
             for (std::size_t i = 0; i < slices.size(); ++i)
             {
               start.push_back(0);
               stop.push_back(0);
               step.push_back(0);
               slicelength.push_back(0);
             }
             for (std::size_t i = 0; i < slices.size(); ++i)
             {
               if (!slices[i].compute(s.shape()[i], &start[i], &stop[i], &step[i], &slicelength[i]))
                 throw py::error_already_set();
             }

             // set up the view to extract from the slices
             NDArrayView arr_view = NDArrayView();
             for (std::size_t i = 0; i < start.size(); ++i)
             {
               arr_view.from.push_back(start[i]);
               arr_view.to.push_back(stop[i]);
               arr_view.step.push_back(step[i]);
             }

             return s.SetRange(arr_view, t);
           })
      .def("__setitem__",
           [](NDArray<T> &s, std::vector<std::vector<std::size_t>> const &idxs,
              NDArray<T> const &t) {
             assert(idxs.size() > 0);
             for (auto cur_idx : idxs)
             {
               assert(cur_idx.size() == 3);
             }

             // set up the view to extract
             NDArrayView arr_view = NDArrayView();
             for (auto item : idxs)
             {
               arr_view.from.push_back(item[0]);
               arr_view.to.push_back(item[1]);
               arr_view.step.push_back(item[2]);
             }

             return s.SetRange(arr_view, t);
           })
      .def("__setitem__",
           [](NDArray<T> &s, std::vector<std::size_t> const &idxs, T val) {
             assert(idxs.size() == s.shape().size());

             return s.Set(idxs, val);
           })
      .def("__setitem__",
           [](NDArray<T> &s, std::size_t idx, T val) {
             if (idx >= s.size()) throw py::index_error();
             s[idx] = val;
           })
      .def("__setitem__",
           [](NDArray<T> &s, std::size_t idx, T val) {
             if (idx >= s.size()) throw py::index_error();
             s[idx] = val;
           })
      .def("Max", [](NDArray<T> const &a) { return a.Max(); })
      //      .def("Max",
      //           [](NDArray<T> const &a, std::size_t const axis) {
      //             if (axis >= a.shape().size()) throw py::index_error();
      //             return a.Max(axis);
      //           })
      .def("Min", [](NDArray<T> const &a) { return a.Min(); })
      //      .def("Min",
      //           [](NDArray<T> const &a, std::size_t axis) {
      //             if (axis >= a.shape().size()) throw py::index_error();
      //             return a.Min(axis);
      //           })
      .def("Log", [](NDArray<T> &a) { return fetch::math::Log(a); })
      .def("Exp", [](NDArray<T> &a) { return fetch::math::Exp(a); })
      .def("Relu", [](NDArray<T> const &a, NDArray<T> &b) { return b.Relu(a); })
      .def("L2Loss", [](NDArray<T> const &a) { return a.L2Loss(); })
      .def("Sign",
           [](NDArray<T> const &a, NDArray<T> &b) {
             std::cout << "entering sign binding.." << std::endl;
             return b.Sign(a);
           })
      .def("Reshape",
           [](NDArray<T> &a, std::vector<std::size_t> b) {
             //             std::size_t internal_prod = std::size_t(std::accumulate(begin(b),
             //             end(b), 1, std::multiplies<>())); if (internal_prod != a.size()) throw
             //             py::index_error();
             if (!(a.CanReshape(b))) throw py::index_error();
             a.Reshape(b);
             return;
           })
      .def("Shape", [](NDArray<T> &a) { return a.shape(); });
  //                  [](NDArray<T> &a, NDArray<T> &b)
  //                  {
  //                    a.Relu(b);
  //                    return a;
  //                  });
}
};  // namespace math
};  // namespace fetch