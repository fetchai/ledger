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
      .def("__truediv__",
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
      .def("__truediv__",
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
      .def("__truediv__",
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
      .def("__truediv__",
           [](NDArray<T> &a, T const &c) {
             a.InlineDivide(c);
             return a;
           })
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
             assert(idxs.size() == s.Shape().size());

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
             assert(idxs.size() == s.Shape().size());

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
      .def("Max",
           [](NDArray<T> const &a, std::size_t axis) {
             if (axis >= a.shape().size()) throw py::index_error();
             return a.Max(axis);
           })
      .def("Min", [](NDArray<T> const &a) { return a.Min(); })
      .def("Min",
           [](NDArray<T> const &a, std::size_t axis) {
             if (axis >= a.shape().size()) throw py::index_error();
             return a.Min(axis);
           })
      .def("Reshape",
           [](NDArray<T> &a, std::vector<std::size_t> b) {
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