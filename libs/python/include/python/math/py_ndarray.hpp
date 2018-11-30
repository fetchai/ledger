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

#include "math/exp.hpp"
#include "math/free_functions/free_functions.hpp"
#include "math/log.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_squeeze.hpp"
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
      .def(py::init<>())
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
      .def("flatten",
           [](NDArray<T> &a) {
             a.Flatten();
             return a;
           })
      .def_static("Zeros", &NDArray<T>::Zeroes)
      .def_static("Ones", &NDArray<T>::Ones)

      // TODO(private issue 188): Move implementation of these functions to ndarray.
      .def("reduce_sum",
           [](NDArray<T> &x, NDArray<T> &y, uint64_t const &axis) {
             if (axis >= x.shape().size())
             {
               throw py::index_error();
             }
             Reduce([](T const &a, T const &b) { return a + b; }, y, x, axis);
           })
      .def("reduce_mean",
           [](NDArray<T> &x, NDArray<T> &y, uint64_t const &axis) {
             if (axis >= y.shape().size())
             {
               throw py::index_error();
             }
             Reduce([](T const &a, T const &b) { return a + b; }, y, x, axis);

             T d = T(1.) / T(y.shape(axis));
             for (std::size_t i = 0; i < x.size(); ++i)
             {
               x[i] *= d;
             }
           })
      .def("transpose",
           [](NDArray<T> &x, NDArray<T> &y, std::vector<uint64_t> const &perm) {
             if (perm.size() != y.shape().size())
             {
               throw py::index_error();
             }
             std::vector<std::size_t> newshape;
             newshape.reserve(y.shape().size());

             for (std::size_t i = 0; i < perm.size(); ++i)
             {
               newshape.push_back(y.shape(perm[i]));
             }

             x.ResizeFromShape(newshape);

             NDArrayIterator<T, typename NDArray<T>::container_type> it(x);
             NDArrayIterator<T, typename NDArray<T>::container_type> it2(y);
             it.ReverseAxes();
             while (bool(it) && bool(it2))
             {
               *it = *it2;
               ++it;
               ++it2;
             }
           })
      .def("reduce_any",
           [](NDArray<T> &x, NDArray<T> &y, uint64_t const &axis) {
             Reduce(
                 [](T const &a, T const &b) {
                   if (a != T(0))
                   {
                     return T(1);
                   }
                   if (b != T(0))
                   {
                     return T(1);
                   }
                   return T(0);
                 },
                 y, x, axis);
           })
      //      .def("expand_dims",[](NDArray<T> &x, NDArray<T> &y, uint64_t const& axis) {
      //      })
      .def("__add__",
           [](NDArray<T> &b, NDArray<T> &c) {
             // identify the correct output shape
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }

             // put result of addition into new output array
             NDArray<T> a{new_shape};
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> &b, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }
             NDArray<T> a{new_shape};
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> &b, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }

             NDArray<T> a{new_shape};
             Subtract(b, c, a);
             return a;
           })
      .def("__truediv__",
           [](NDArray<T> &b, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }

             NDArray<T> a{new_shape};
             Divide(b, c, a);
             return a;
           })
      .def("__add__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a{b.size()};
             //             if (a.CanReshape(b))
             //             {
             //
             //             }
             a.LazyReshape(b.shape());
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             Subtract(b, c, a);
             return a;
           })
      .def("__truediv__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             Divide(b, c, a);
             return a;
           })
      .def("__iadd__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }
             if (new_shape != a.shape())
             {
               py::print("broadcast shape (", new_shape, ") does not match shape of output array (",
                         a.shape(), ")");
               throw py::value_error();
             }
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }
             if (new_shape != a.shape())
             {
               py::print("broadcast shape (", new_shape, ") does not match shape of output array (",
                         a.shape(), ")");
               throw py::value_error();
             }

             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }
             if (new_shape != a.shape())
             {
               py::print("broadcast shape (", new_shape, ") does not match shape of output array (",
                         a.shape(), ")");
               throw py::value_error();
             }

             a.InlineSubtract(c);
             return a;
           })
      .def("__itruediv__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape)))
             {
               throw py::index_error();
             }
             if (new_shape != a.shape())
             {
               py::print("broadcast shape (", new_shape, ") does not match shape of output array (",
                         a.shape(), ")");
               throw py::value_error();
             }

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
      .def("__itruediv__",
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
             if (idx >= s.size())
             {
               throw py::index_error();
             }
             return s[idx];
           })
      .def("__getitem__",
           [](NDArray<T> &a, std::vector<py::slice> slices) {
             // std::vector<size_t> start, stop, step, slicelength;
             std::vector<std::vector<std::size_t>> range;
             range.resize(slices.size());

             for (std::size_t i = 0; i < slices.size(); ++i)
             {
               range[i].resize(3);
               //               slicelength.push_back(0);
             }
             std::vector<std::size_t> newshape;

             for (std::size_t i = 0; i < slices.size(); ++i)
             {
               std::size_t s;
               if (!slices[i].compute(a.shape()[i], &range[i][0], &range[i][1], &range[i][2], &s))
               {
                 throw py::error_already_set();
               }
               newshape.push_back(s);
             }

             NDArray<T>                                              ret(newshape);
             NDArrayIterator<T, typename NDArray<T>::container_type> it(a, range);
             NDArrayIterator<T, typename NDArray<T>::container_type> it2(ret);
             while (bool(it) && bool(it2))
             {
               *it2 = *it;
               ++it;
               ++it2;
             }
             return ret;
             /*
                          // set up the view to extract
                          NDArrayView arr_view = NDArrayView();
                          for (std::size_t i = 0; i < start.size(); ++i)
                          {
                            arr_view.from.push_back(start[i]);
                            arr_view.to.push_back(stop[i]);
                            arr_view.step.push_back(step[i]);
                          }

                          return s.GetRange(arr_view);
             */
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
               {
                 throw py::error_already_set();
               }
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
             if (idx >= s.size())
             {
               throw py::index_error();
             }
             s[idx] = val;
           })
      .def("max",
           [](NDArray<T> &a) {
             typename NDArray<T>::Type ret = -std::numeric_limits<typename NDArray<T>::Type>::max();
             Max(a, ret);
             return a;
           })
      .def("max",
           [](NDArray<T> &a, std::size_t const axis) {
             if (axis >= a.shape().size())
             {
               throw py::index_error();
             }

             std::vector<std::size_t> return_shape{a.shape()};
             return_shape.erase(return_shape.begin() + int(axis),
                                return_shape.begin() + int(axis) + 1);
             NDArray<T> ret{return_shape};

             Max(a, axis, ret);
             return ret;
           })
      .def("maximum",
           [](NDArray<T> &ret, NDArray<T> const &array1, NDArray<T> const &array2) {
             Maximum(array1, array2, ret);
             return ret;
           })
      .def("min",
           [](NDArray<T> const &a) {
             typename NDArray<T>::Type ret = std::numeric_limits<typename NDArray<T>::Type>::max();
             Min(a, ret);
             return a;
           })
      .def("min",
           [](NDArray<T> &a, std::size_t const axis) {
             if (axis >= a.shape().size())
             {
               throw py::index_error();
             }

             std::vector<std::size_t> return_shape{a.shape()};
             return_shape.erase(return_shape.begin() + int(axis),
                                return_shape.begin() + int(axis) + 1);
             NDArray<T> ret{return_shape};

             Min(a, axis, ret);
             return ret;
           })
      .def("relu",
           [](NDArray<T> &a, NDArray<T> const &b) {
             a = b;
             Relu(a);
             return a;
           })
      .def("l2loss", [](NDArray<T> const &a) { return a.L2Loss(); })
      .def("sign_functionality",
           [](NDArray<T> &a, NDArray<T> const &b) {
             a = b;
             Sign(a);
             return a;
           })
      .def("reshape",
           [](NDArray<T> &a, std::vector<std::size_t> b, bool flip_order) {
             if (!(a.CanReshape(b)))
             {
               py::print("cannot reshape array of size (", a.size(), ") into shape of(", b, ")");
               throw py::value_error();
             }

             if (flip_order)
             {
               a.MajorOrderFlip();
               a.Reshape(b);
               a.MajorOrderFlip();
             }
             else
             {
               a.Reshape(b);
             }
             return;
           })
      .def("boolean_mask",
           [](NDArray<T> &a, NDArray<T> &mask) {
             NDArray<T> ret{std::size_t(Sum(mask))};
             fetch::math::BooleanMask(a, mask, ret);
             return ret;
           })
      .def("dynamic_stitch",
           [](NDArray<T> &input_array, NDArray<T> &indices, NDArray<T> const &data) {
             DynamicStitch(input_array, indices, data);
             return input_array;
           })
      .def("shape", [](NDArray<T> &a) { return a.shape(); })
      .def_static("Zeros", &NDArray<T>::Zeroes)

      // various free functions
      .def("abs", [](NDArray<T> &a) { fetch::math::Abs(a); })
      .def("exp", [](NDArray<T> &a) { fetch::math::Exp(a); })
      .def("exp2", [](NDArray<T> &a) { fetch::math::Exp2(a); })
      .def("expm1", [](NDArray<T> &a) { fetch::math::Expm1(a); })
      .def("log", [](NDArray<T> &a) { fetch::math::Log(a); })
      .def("log10", [](NDArray<T> &a) { fetch::math::Log10(a); })
      .def("log2", [](NDArray<T> &a) { fetch::math::Log2(a); })
      .def("log1p", [](NDArray<T> &a) { fetch::math::Log1p(a); })
      .def("sqrt", [](NDArray<T> &a) { fetch::math::Sqrt(a); })
      .def("cbrt", [](NDArray<T> &a) { fetch::math::Cbrt(a); })
      .def("sin", [](NDArray<T> &a) { fetch::math::Sin(a); })
      .def("cos", [](NDArray<T> &a) { fetch::math::Cos(a); })
      .def("tan", [](NDArray<T> &a) { fetch::math::Tan(a); })
      .def("asin", [](NDArray<T> &a) { fetch::math::Asin(a); })
      .def("acos", [](NDArray<T> &a) { fetch::math::Acos(a); })
      .def("atan", [](NDArray<T> &a) { fetch::math::Atan(a); })
      .def("sinh", [](NDArray<T> &a) { fetch::math::Sinh(a); })
      .def("cosh", [](NDArray<T> &a) { fetch::math::Cosh(a); })
      .def("tanh", [](NDArray<T> &a) { fetch::math::Tanh(a); })
      .def("asinh", [](NDArray<T> &a) { fetch::math::Asinh(a); })
      .def("acosh", [](NDArray<T> &a) { fetch::math::Acosh(a); })
      .def("atanh", [](NDArray<T> &a) { fetch::math::Atanh(a); })
      .def("erf", [](NDArray<T> &a) { fetch::math::Erf(a); })
      .def("erfc", [](NDArray<T> &a) { fetch::math::Erfc(a); })
      .def("tgamma", [](NDArray<T> &a) { fetch::math::Tgamma(a); })
      .def("lgamma", [](NDArray<T> &a) { fetch::math::Lgamma(a); })
      .def("ceil", [](NDArray<T> &a) { fetch::math::Ceil(a); })
      .def("floor", [](NDArray<T> &a) { fetch::math::Floor(a); })
      .def("trunc", [](NDArray<T> &a) { fetch::math::Trunc(a); })
      .def("round", [](NDArray<T> &a) { fetch::math::Round(a); })
      .def("lround", [](NDArray<T> &a) { fetch::math::Lround(a); })
      .def("llround", [](NDArray<T> &a) { fetch::math::Llround(a); })
      .def("nearbyint", [](NDArray<T> &a) { fetch::math::Nearbyint(a); })
      .def("rint", [](NDArray<T> &a) { fetch::math::Rint(a); })
      .def("lrint", [](NDArray<T> &a) { fetch::math::Lrint(a); })
      .def("llrint", [](NDArray<T> &a) { fetch::math::Llrint(a); })
      .def("isfinite", [](NDArray<T> &a) { fetch::math::Isfinite(a); })
      .def("isinf", [](NDArray<T> &a) { fetch::math::Isinf(a); })
      .def("isnan", [](NDArray<T> &a) { fetch::math::Isnan(a); })
      .def("approx_exp", [](NDArray<T> &a) { fetch::math::ApproxExp(a); })
      .def("approx_log", [](NDArray<T> &a) { fetch::math::ApproxLog(a); })
      .def("approx_logistic", [](NDArray<T> &a) { fetch::math::ApproxLogistic(a); })

      .def("relu", [](NDArray<T> &a) { fetch::math::Relu(a); })
      .def("sign_functionality", [](NDArray<T> &a) { fetch::math::Sign(a); })

      .def("scatter",
           [](NDArray<T> &input_array, NDArray<T> &updates, NDArray<T> &indices) {
             fetch::math::Scatter(input_array, updates, indices);
             return input_array;
           })
      .def("gather",
           [](NDArray<T> &input_array, NDArray<T> &updates, NDArray<T> &indices) {
             fetch::math::Gather(input_array, updates, indices);
             return input_array;
           })
      .def("transpose",
           [](NDArray<T> &input_array, std::vector<std::size_t> &perm) {
             fetch::math::Transpose(input_array, perm);
             return input_array;
           })
      .def("concat",
           [](NDArray<T> &input_array, std::vector<NDArray<T>> concat_arrays, std::size_t axis) {
             fetch::math::Concat(input_array, concat_arrays, axis);
             return input_array;
           })
      .def("expand_dims",
           [](NDArray<T> &input_array, int const &axis) {
             fetch::math::ExpandDimensions(input_array, axis);
             return input_array;
           })
      //      .def("softmax",
      //           [](NDArray<T> &array, NDArray<T> &ret) {
      //             Softmax(array, ret);
      //             return ret;
      //           })
      .def("major_order_flip", [](NDArray<T> &array) { array.MajorOrderFlip(); })
      .def("major_order",
           [](NDArray<T> &array) {
             if (array.MajorOrder() == NDArray<T>::MAJOR_ORDER::COLUMN)
             {
               return "column";
             }
             else
             {
               return "row";
             }
           })
      .def("FromNumpy",
           [](NDArray<T> &s, py::array_t<T> arr) {
             using size_type = typename NDArray<T>::size_type;
             auto buf        = arr.request();

             T *ptr = (T *)buf.ptr;

             // get shape of the numpy array
             std::vector<std::size_t> shape;
             std::vector<std::size_t> stride;
             std::vector<std::size_t> index;
             for (std::size_t i = 0; i < buf.shape.size(); ++i)
             {
               shape.push_back(size_type(buf.shape[i]));
               stride.push_back(std::size_t(buf.strides[i]) / sizeof(T));
               index.push_back(0);
             }

             s.CopyFromNumpy(ptr, shape, stride, index);
           })
      .def("ToNumpy", [](NDArray<T> &s) {
        std::vector<std::size_t> shape  = s.shape();
        auto                     result = py::array_t<T>(shape);
        auto                     buf    = result.request();

        std::vector<std::size_t> stride;
        std::vector<std::size_t> index;
        for (std::size_t i = 0; i < buf.shape.size(); ++i)
        {
          stride.push_back(std::size_t(buf.strides[i]) / sizeof(T));
          index.push_back(0);
        }

        T *ptr = (T *)buf.ptr;

        s.CopyToNumpy(ptr, shape, stride, index);

        return result;
      });
}

}  // namespace math
}  // namespace fetch
