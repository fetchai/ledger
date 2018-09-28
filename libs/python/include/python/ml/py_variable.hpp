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

#include "ml/ops/ops.hpp"
#include "ml/session.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>

namespace fetch {
namespace ml {

template <typename ArrayType>
void BuildVariable(std::string const &custom_name, pybind11::module &module)
{

  using SelfType    = fetch::ml::Variable<ArrayType>;
  using SessionType = fetch::ml::SessionManager<ArrayType, SelfType>;

  namespace py = pybind11;
  py::class_<SelfType>(module, custom_name.c_str())
      //      .def(py::init<>())
      //      .def(py::init<std::size_t const &>())
      //      .def(py::init<std::vector<std::size_t> const &>())
      .def(py::init<std::size_t const &, std::size_t const &, SessionType &>())
      .def("Dot", [](SelfType &a, SelfType &b,
                     SessionType &sess) { return fetch::ml::ops::Dot(a, b, sess); })
      .def("Relu", [](SelfType &a, SessionType &sess) { return fetch::ml::ops::Relu(a, sess); })
      .def("ReduceSum", [](SelfType &a, std::size_t axis,
                           SessionType &sess) { return fetch::ml::ops::Sum(a, axis, sess); })
      .def("size", [](SelfType &a) { return a.size(); })
      .def_static("zeroes", [](std::vector<std::size_t> const &new_shape,
                               SessionType &sess) { return SelfType::Zeroes(new_shape, sess); })
      .def("data", [](SelfType &a) { return a.data; })
      .def("__getitem__",
           [](const SelfType &s, std::size_t i) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             return s[i];
           })
      .def("__setitem__", [](SelfType &s, std::size_t i, typename ArrayType::type v) {
        if (i >= s.size())
        {
          throw py::index_error();
        }
        s[i] = v;
      });

  //  .def("Copy", [](NDArray<ArrayType> &a) { return a.Copy(); })
  //      .def("Copy",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> &b) {
  //             assert(a.size() == b.size());
  //             assert(a.shape() == b.shape());
  //             a.Copy(b);
  //             return a;
  //           })
  //      .def("flatten",
  //           [](NDArray<ArrayType> &a) {
  //             a.Flatten();
  //             return a;
  //           })
  //      .def_static("Zeros", &NDArray<ArrayType>::Zeroes)
  //      .def_static("Ones", &NDArray<ArrayType>::Ones)
  //
  //          // TODO(private issue 188): Move implementation of these functions to ndarray.
  //      .def("reduce_sum",
  //           [](NDArray<ArrayType> &x, NDArray<ArrayType> &y, uint64_t const &axis) {
  //             if (axis >= x.shape().size())
  //             {
  //               throw py::index_error();
  //             }
  //             Reduce([](T const &a, T const &b) { return a + b; }, y, x, axis);
  //           })
  //      .def("reduce_mean",
  //           [](NDArray<ArrayType> &x, NDArray<ArrayType> &y, uint64_t const &axis) {
  //             if (axis >= y.shape().size())
  //             {
  //               throw py::index_error();
  //             }
  //             Reduce([](T const &a, T const &b) { return a + b; }, y, x, axis);
  //
  //             T d = T(1.) / T(y.shape(axis));
  //             for (std::size_t i = 0; i < x.size(); ++i)
  //             {
  //               x[i] *= d;
  //             }
  //           })
  //      .def("transpose",
  //           [](NDArray<ArrayType> &x, NDArray<ArrayType> &y, std::vector<uint64_t> const &perm) {
  //             if (perm.size() != y.shape().size())
  //             {
  //               throw py::index_error();
  //             }
  //             std::vector<std::size_t> newshape;
  //             newshape.reserve(y.shape().size());
  //
  //             for (std::size_t i = 0; i < perm.size(); ++i)
  //             {
  //               newshape.push_back(y.shape(perm[i]));
  //             }
  //
  //             x.ResizeFromShape(newshape);
  //
  //             NDArrayIterator<T, typename NDArray<ArrayType>::container_type> it(x);
  //             NDArrayIterator<T, typename NDArray<ArrayType>::container_type> it2(y);
  //             it.ReverseAxes();
  //             while (bool(it) && bool(it2))
  //             {
  //               *it = *it2;
  //               ++it;
  //               ++it2;
  //             }
  //           })
  //      .def("reduce_any",
  //           [](NDArray<ArrayType> &x, NDArray<ArrayType> &y, uint64_t const &axis) {
  //             Reduce(
  //                 [](T const &a, T const &b) {
  //                   if (a != T(0))
  //                   {
  //                     return T(1);
  //                   }
  //                   if (b != T(0))
  //                   {
  //                     return T(1);
  //                   }
  //                   return T(0);
  //                 },
  //                 y, x, axis);
  //           })
  //          //      .def("expand_dims",[](NDArray<ArrayType> &x, NDArray<ArrayType> &y, uint64_t
  //          const& axis) {
  //          //      })
  //      .def("__add__",
  //           [](NDArray<ArrayType> &b, NDArray<ArrayType> &c) {
  //             // identify the correct output shape
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //
  //             // put result of addition into new output array
  //             NDArray<ArrayType> a{new_shape};
  //             Add(b, c, a);
  //             return a;
  //           })
  //      .def("__mul__",
  //           [](NDArray<ArrayType> &b, NDArray<ArrayType> &c) {
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //             NDArray<ArrayType> a{new_shape};
  //             Multiply(b, c, a);
  //             return a;
  //           })
  //      .def("__sub__",
  //           [](NDArray<ArrayType> &b, NDArray<ArrayType> &c) {
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //
  //             NDArray<ArrayType> a{new_shape};
  //             Subtract(b, c, a);
  //             return a;
  //           })
  //      .def("__truediv__",
  //           [](NDArray<ArrayType> &b, NDArray<ArrayType> &c) {
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //
  //             NDArray<ArrayType> a{new_shape};
  //             Divide(b, c, a);
  //             return a;
  //           })
  //      .def("__add__",
  //           [](NDArray<ArrayType> &b, T const &c) {
  //             NDArray<ArrayType> a{b.size()};
  //             //             if (a.CanReshape(b))
  //             //             {
  //             //
  //             //             }
  //             a.LazyReshape(b.shape());
  //             Add(b, c, a);
  //             return a;
  //           })
  //      .def("__mul__",
  //           [](NDArray<ArrayType> &b, T const &c) {
  //             NDArray<ArrayType> a(b.size());
  //             a.LazyReshape(b.shape());
  //             Multiply(b, c, a);
  //             return a;
  //           })
  //      .def("__sub__",
  //           [](NDArray<ArrayType> &b, T const &c) {
  //             NDArray<ArrayType> a(b.size());
  //             a.LazyReshape(b.shape());
  //             Subtract(b, c, a);
  //             return a;
  //           })
  //      .def("__truediv__",
  //           [](NDArray<ArrayType> &b, T const &c) {
  //             NDArray<ArrayType> a(b.size());
  //             a.LazyReshape(b.shape());
  //             Divide(b, c, a);
  //             return a;
  //           })
  //      .def("__iadd__",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> &c) {
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //             if (new_shape != a.shape())
  //             {
  //               py::print("broadcast shape (", new_shape, ") does not match shape of output array
  //               (",
  //                         a.shape(), ")");
  //               throw py::value_error();
  //             }
  //             a.InlineAdd(c);
  //             return a;
  //           })
  //      .def("__imul__",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> &c) {
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //             if (new_shape != a.shape())
  //             {
  //               py::print("broadcast shape (", new_shape, ") does not match shape of output array
  //               (",
  //                         a.shape(), ")");
  //               throw py::value_error();
  //             }
  //
  //             a.InlineMultiply(c);
  //             return a;
  //           })
  //      .def("__isub__",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> &c) {
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //             if (new_shape != a.shape())
  //             {
  //               py::print("broadcast shape (", new_shape, ") does not match shape of output array
  //               (",
  //                         a.shape(), ")");
  //               throw py::value_error();
  //             }
  //
  //             a.InlineSubtract(c);
  //             return a;
  //           })
  //      .def("__itruediv__",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> &c) {
  //             std::vector<std::size_t> new_shape;
  //             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
  //             {
  //               throw py::index_error();
  //             }
  //             if (new_shape != a.shape())
  //             {
  //               py::print("broadcast shape (", new_shape, ") does not match shape of output array
  //               (",
  //                         a.shape(), ")");
  //               throw py::value_error();
  //             }
  //
  //             a.InlineDivide(c);
  //             return a;
  //           })
  //      .def("__iadd__",
  //           [](NDArray<ArrayType> &a, T const &c) {
  //             a.InlineAdd(c);
  //             return a;
  //           })
  //      .def("__imul__",
  //           [](NDArray<ArrayType> &a, T const &c) {
  //             a.InlineMultiply(c);
  //             return a;
  //           })
  //      .def("__isub__",
  //           [](NDArray<ArrayType> &a, T const &c) {
  //             a.InlineSubtract(c);
  //             return a;
  //           })
  //      .def("__itruediv__",
  //           [](NDArray<ArrayType> &a, T const &c) {
  //             a.InlineDivide(c);
  //             return a;
  //           })
  //      .def("__eq__",
  //           [](NDArray<ArrayType> const &s, NDArray<ArrayType> const &other) { return
  //           s.operator==(other); })
  //      .def("__ne__",
  //           [](NDArray<ArrayType> const &s, NDArray<ArrayType> const &other) { return
  //           s.operator!=(other); })
  //      .def("__getitem__",
  //           [](NDArray<ArrayType> const &s, std::size_t idx) {
  //             if (idx >= s.size())
  //             {
  //               throw py::index_error();
  //             }
  //             return s[idx];
  //           })
  //      .def("__getitem__",
  //           [](NDArray<ArrayType> &a, std::vector<py::slice> slices) {
  //             // std::vector<size_t> start, stop, step, slicelength;
  //             std::vector<std::vector<std::size_t>> range;
  //             range.resize(slices.size());
  //
  //             for (std::size_t i = 0; i < slices.size(); ++i)
  //             {
  //               range[i].resize(3);
  //               //               slicelength.push_back(0);
  //             }
  //             std::vector<std::size_t> newshape;
  //
  //             for (std::size_t i = 0; i < slices.size(); ++i)
  //             {
  //               std::size_t s;
  //               if (!slices[i].compute(a.shape()[i], &range[i][0], &range[i][1], &range[i][2],
  //               &s))
  //               {
  //                 throw py::error_already_set();
  //               }
  //               newshape.push_back(s);
  //             }
  //
  //             NDArray<ArrayType>                                              ret(newshape);
  //             NDArrayIterator<T, typename NDArray<ArrayType>::container_type> it(a, range);
  //             NDArrayIterator<T, typename NDArray<ArrayType>::container_type> it2(ret);
  //             while (bool(it) && bool(it2))
  //             {
  //               *it2 = *it;
  //               ++it;
  //               ++it2;
  //             }
  //             return ret;
  //             /*
  //                          // set up the view to extract
  //                          NDArrayView arr_view = NDArrayView();
  //                          for (std::size_t i = 0; i < start.size(); ++i)
  //                          {
  //                            arr_view.from.push_back(start[i]);
  //                            arr_view.to.push_back(stop[i]);
  //                            arr_view.step.push_back(step[i]);
  //                          }
  //
  //                          return s.GetRange(arr_view);
  //             */
  //           })
  //      .def("__getitem__",
  //           [](NDArray<ArrayType> const &s, std::vector<std::vector<std::size_t>> const &idxs) {
  //             assert(idxs.size() > 0);
  //             for (auto cur_idx : idxs)
  //             {
  //               assert(cur_idx.size() == 3);
  //             }
  //
  //             // set up the view to extract
  //             NDArrayView arr_view = NDArrayView();
  //             for (auto item : idxs)
  //             {
  //               arr_view.from.push_back(item[0]);
  //               arr_view.to.push_back(item[1]);
  //               arr_view.step.push_back(item[2]);
  //             }
  //
  //             return s.GetRange(arr_view);
  //           })
  //      .def("__getitem__",
  //           [](NDArray<ArrayType> const &s, std::vector<std::size_t> const &idxs) {
  //             assert(idxs.size() == s.shape().size());
  //             return s.Get(idxs);
  //           })
  //      .def("__setitem__",
  //           [](NDArray<ArrayType> &s, std::vector<py::slice> slices, NDArray<ArrayType> const &t)
  //           {
  //             // manage slice assignment
  //             std::vector<size_t> start, stop, step, slicelength;
  //             for (std::size_t i = 0; i < slices.size(); ++i)
  //             {
  //               start.push_back(0);
  //               stop.push_back(0);
  //               step.push_back(0);
  //               slicelength.push_back(0);
  //             }
  //             for (std::size_t i = 0; i < slices.size(); ++i)
  //             {
  //               if (!slices[i].compute(s.shape()[i], &start[i], &stop[i], &step[i],
  //               &slicelength[i]))
  //               {
  //                 throw py::error_already_set();
  //               }
  //             }
  //
  //             // set up the view to extract from the slices
  //             NDArrayView arr_view = NDArrayView();
  //             for (std::size_t i = 0; i < start.size(); ++i)
  //             {
  //               arr_view.from.push_back(start[i]);
  //               arr_view.to.push_back(stop[i]);
  //               arr_view.step.push_back(step[i]);
  //             }
  //
  //             return s.SetRange(arr_view, t);
  //           })
  //      .def("__setitem__",
  //           [](NDArray<ArrayType> &s, std::vector<std::vector<std::size_t>> const &idxs,
  //              NDArray<ArrayType> const &t) {
  //             assert(idxs.size() > 0);
  //             for (auto cur_idx : idxs)
  //             {
  //               assert(cur_idx.size() == 3);
  //             }
  //
  //             // set up the view to extract
  //             NDArrayView arr_view = NDArrayView();
  //             for (auto item : idxs)
  //             {
  //               arr_view.from.push_back(item[0]);
  //               arr_view.to.push_back(item[1]);
  //               arr_view.step.push_back(item[2]);
  //             }
  //
  //             return s.SetRange(arr_view, t);
  //           })
  //      .def("__setitem__",
  //           [](NDArray<ArrayType> &s, std::vector<std::size_t> const &idxs, T val) {
  //             assert(idxs.size() == s.shape().size());
  //
  //             return s.Set(idxs, val);
  //           })
  //      .def("__setitem__",
  //           [](NDArray<ArrayType> &s, std::size_t idx, T val) {
  //             if (idx >= s.size())
  //             {
  //               throw py::index_error();
  //             }
  //             s[idx] = val;
  //           })
  //      .def("__setitem__",
  //           [](NDArray<ArrayType> &s, std::size_t idx, T val) {
  //             if (idx >= s.size())
  //             {
  //               throw py::index_error();
  //             }
  //             s[idx] = val;
  //           })
  //      .def("max",
  //           [](NDArray<ArrayType> &a) {
  //             typename NDArray<ArrayType>::type ret = -std::numeric_limits<typename
  //             NDArray<ArrayType>::type>::max(); Max(a, ret); return a;
  //           })
  //      .def("max",
  //           [](NDArray<ArrayType> &a, std::size_t const axis) {
  //             if (axis >= a.shape().size())
  //             {
  //               throw py::index_error();
  //             }
  //
  //             std::vector<std::size_t> return_shape{a.shape()};
  //             return_shape.erase(return_shape.begin() + int(axis),
  //                                return_shape.begin() + int(axis) + 1);
  //             NDArray<ArrayType> ret{return_shape};
  //
  //             Max(a, axis, ret);
  //             return ret;
  //           })
  //      .def("maximum",
  //           [](NDArray<ArrayType> &ret, NDArray<ArrayType> const &array1, NDArray<ArrayType>
  //           const &array2) {
  //             Maximum(array1, array2, ret);
  //             return ret;
  //           })
  //      .def("min",
  //           [](NDArray<ArrayType> const &a) {
  //             typename NDArray<ArrayType>::type ret = std::numeric_limits<typename
  //             NDArray<ArrayType>::type>::max(); Min(a, ret); return a;
  //           })
  //      .def("min",
  //           [](NDArray<ArrayType> &a, std::size_t const axis) {
  //             if (axis >= a.shape().size())
  //             {
  //               throw py::index_error();
  //             }
  //
  //             std::vector<std::size_t> return_shape{a.shape()};
  //             return_shape.erase(return_shape.begin() + int(axis),
  //                                return_shape.begin() + int(axis) + 1);
  //             NDArray<ArrayType> ret{return_shape};
  //
  //             Min(a, axis, ret);
  //             return ret;
  //           })
  //      .def("relu",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> const &b) {
  //             a = b;
  //             Relu(a);
  //             return a;
  //           })
  //      .def("l2loss", [](NDArray<ArrayType> const &a) { return a.L2Loss(); })
  //      .def("sign",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> const &b) {
  //             a = b;
  //             Sign(a);
  //             return a;
  //           })
  //      .def("reshape",
  //           [](NDArray<ArrayType> &a, std::vector<std::size_t> b, bool flip_order) {
  //             if (!(a.CanReshape(b)))
  //             {
  //               py::print("cannot reshape array of size (", a.size(), ") into shape of(", b,
  //               ")"); throw py::value_error();
  //             }
  //
  //             if (flip_order)
  //             {
  //               a.MajorOrderFlip();
  //               a.Reshape(b);
  //               a.MajorOrderFlip();
  //             }
  //             else
  //             {
  //               a.Reshape(b);
  //             }
  //             return;
  //           })
  //      .def("boolean_mask",
  //           [](NDArray<ArrayType> &a, NDArray<ArrayType> &mask) {
  //             NDArray<ArrayType> ret{std::size_t(Sum(mask))};
  //             fetch::math::BooleanMask(a, mask, ret);
  //             return ret;
  //           })
  //      .def("dynamic_stitch",
  //           [](NDArray<ArrayType> &input_array, NDArray<ArrayType> &indices, NDArray<ArrayType>
  //           const &data) {
  //             DynamicStitch(input_array, indices, data);
  //             return input_array;
  //           })
  //      .def("shape", [](NDArray<ArrayType> &a) { return a.shape(); })
  //      .def_static("Zeros", &NDArray<ArrayType>::Zeroes)
  //
  //          // various free functions
  //      .def("abs", [](NDArray<ArrayType> &a) { fetch::math::Abs(a); })
  //      .def("exp", [](NDArray<ArrayType> &a) { fetch::math::Exp(a); })
  //      .def("exp2", [](NDArray<ArrayType> &a) { fetch::math::Exp2(a); })
  //      .def("expm1", [](NDArray<ArrayType> &a) { fetch::math::Expm1(a); })
  //      .def("log", [](NDArray<ArrayType> &a) { fetch::math::Log(a); })
  //      .def("log10", [](NDArray<ArrayType> &a) { fetch::math::Log10(a); })
  //      .def("log2", [](NDArray<ArrayType> &a) { fetch::math::Log2(a); })
  //      .def("log1p", [](NDArray<ArrayType> &a) { fetch::math::Log1p(a); })
  //      .def("sqrt", [](NDArray<ArrayType> &a) { fetch::math::Sqrt(a); })
  //      .def("cbrt", [](NDArray<ArrayType> &a) { fetch::math::Cbrt(a); })
  //      .def("sin", [](NDArray<ArrayType> &a) { fetch::math::Sin(a); })
  //      .def("cos", [](NDArray<ArrayType> &a) { fetch::math::Cos(a); })
  //      .def("tan", [](NDArray<ArrayType> &a) { fetch::math::Tan(a); })
  //      .def("asin", [](NDArray<ArrayType> &a) { fetch::math::Asin(a); })
  //      .def("acos", [](NDArray<ArrayType> &a) { fetch::math::Acos(a); })
  //      .def("atan", [](NDArray<ArrayType> &a) { fetch::math::Atan(a); })
  //      .def("sinh", [](NDArray<ArrayType> &a) { fetch::math::Sinh(a); })
  //      .def("cosh", [](NDArray<ArrayType> &a) { fetch::math::Cosh(a); })
  //      .def("tanh", [](NDArray<ArrayType> &a) { fetch::math::Tanh(a); })
  //      .def("asinh", [](NDArray<ArrayType> &a) { fetch::math::Asinh(a); })
  //      .def("acosh", [](NDArray<ArrayType> &a) { fetch::math::Acosh(a); })
  //      .def("atanh", [](NDArray<ArrayType> &a) { fetch::math::Atanh(a); })
  //      .def("erf", [](NDArray<ArrayType> &a) { fetch::math::Erf(a); })
  //      .def("erfc", [](NDArray<ArrayType> &a) { fetch::math::Erfc(a); })
  //      .def("tgamma", [](NDArray<ArrayType> &a) { fetch::math::Tgamma(a); })
  //      .def("lgamma", [](NDArray<ArrayType> &a) { fetch::math::Lgamma(a); })
  //      .def("ceil", [](NDArray<ArrayType> &a) { fetch::math::Ceil(a); })
  //      .def("floor", [](NDArray<ArrayType> &a) { fetch::math::Floor(a); })
  //      .def("trunc", [](NDArray<ArrayType> &a) { fetch::math::Trunc(a); })
  //      .def("round", [](NDArray<ArrayType> &a) { fetch::math::Round(a); })
  //      .def("lround", [](NDArray<ArrayType> &a) { fetch::math::Lround(a); })
  //      .def("llround", [](NDArray<ArrayType> &a) { fetch::math::Llround(a); })
  //      .def("nearbyint", [](NDArray<ArrayType> &a) { fetch::math::Nearbyint(a); })
  //      .def("rint", [](NDArray<ArrayType> &a) { fetch::math::Rint(a); })
  //      .def("lrint", [](NDArray<ArrayType> &a) { fetch::math::Lrint(a); })
  //      .def("llrint", [](NDArray<ArrayType> &a) { fetch::math::Llrint(a); })
  //      .def("isfinite", [](NDArray<ArrayType> &a) { fetch::math::Isfinite(a); })
  //      .def("isinf", [](NDArray<ArrayType> &a) { fetch::math::Isinf(a); })
  //      .def("isnan", [](NDArray<ArrayType> &a) { fetch::math::Isnan(a); })
  //      .def("approx_exp", [](NDArray<ArrayType> &a) { fetch::math::ApproxExp(a); })
  //      .def("approx_log", [](NDArray<ArrayType> &a) { fetch::math::ApproxLog(a); })
  //      .def("approx_logistic", [](NDArray<ArrayType> &a) { fetch::math::ApproxLogistic(a); })
  //
  //      .def("relu", [](NDArray<ArrayType> &a) { fetch::math::Relu(a); })
  //      .def("sign", [](NDArray<ArrayType> &a) { fetch::math::Sign(a); })
  //
  //      .def("scatter",
  //           [](NDArray<ArrayType> &input_array, NDArray<ArrayType> &updates, NDArray<ArrayType>
  //           &indices) {
  //             fetch::math::Scatter(input_array, updates, indices);
  //             return input_array;
  //           })
  //      .def("gather",
  //           [](NDArray<ArrayType> &input_array, NDArray<ArrayType> &updates, NDArray<ArrayType>
  //           &indices) {
  //             fetch::math::Gather(input_array, updates, indices);
  //             return input_array;
  //           })
  //      .def("transpose",
  //           [](NDArray<ArrayType> &input_array, std::vector<std::size_t> &perm) {
  //             fetch::math::Transpose(input_array, perm);
  //             return input_array;
  //           })
  //      .def("concat",
  //           [](NDArray<ArrayType> &input_array, std::vector<NDArray<ArrayType>> concat_arrays,
  //           std::size_t axis)
  //           {
  //             fetch::math::Concat(input_array, concat_arrays, axis);
  //             return input_array;
  //           })
  //      .def("expand_dims",
  //           [](NDArray<ArrayType> &input_array, int const &axis) {
  //             fetch::math::ExpandDimensions(input_array, axis);
  //             return input_array;
  //           })
  //      .def("softmax",
  //           [](NDArray<ArrayType> const &array, NDArray<ArrayType> &ret) {
  //             Softmax(array, ret);
  //             return ret;
  //           })
  //      .def("major_order_flip", [](NDArray<ArrayType> &array) { array.MajorOrderFlip(); })
  //      .def("major_order",
  //           [](NDArray<ArrayType> &array) {
  //             if (array.MajorOrder() == NDArray<ArrayType>::MAJOR_ORDER::COLUMN)
  //             {
  //               return "column";
  //             }
  //             else
  //             {
  //               return "row";
  //             }
  //           })
  //      .def("FromNumpy",
  //           [](NDArray<ArrayType> &s, py::array_t<ArrayType> arr) {
  //             using size_type = typename NDArray<ArrayType>::size_type;
  //             auto buf        = arr.request();
  //
  //             T *ptr = (T *)buf.ptr;
  //
  //             // get shape of the numpy array
  //             std::vector<std::size_t> shape;
  //             std::vector<std::size_t> stride;
  //             std::vector<std::size_t> index;
  //             for (std::size_t i = 0; i < buf.shape.size(); ++i)
  //             {
  //               shape.push_back(size_type(buf.shape[i]));
  //               stride.push_back(std::size_t(buf.strides[i]) / sizeof(T));
  //               index.push_back(0);
  //             }
  //
  //             s.CopyFromNumpy(ptr, shape, stride, index);
  //           })
  //      .def("ToNumpy", [](NDArray<ArrayType> &s) {
  //        std::vector<std::size_t> shape  = s.shape();
  //        auto                     result = py::array_t<ArrayType>(shape);
  //        auto                     buf    = result.request();
  //
  //        std::vector<std::size_t> stride;
  //        std::vector<std::size_t> index;
  //        for (std::size_t i = 0; i < buf.shape.size(); ++i)
  //        {
  //          stride.push_back(std::size_t(buf.strides[i]) / sizeof(T));
  //          index.push_back(0);
  //        }
  //
  //        T *ptr = (T *)buf.ptr;
  //
  //        s.CopyToNumpy(ptr, shape, stride, index);
  //
  //        return result;
  //      });
}

}  // namespace ml
}  // namespace fetch
