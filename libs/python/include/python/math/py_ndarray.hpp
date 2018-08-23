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
      .def("flatten",
           [](NDArray<T> &a) {
             a.Flatten();
             return a;
           })
      .def_static("Zeros", &NDArray<T>::Zeroes)
      .def_static("Ones", &NDArray<T>::Ones)
      .def("__add__",
           [](NDArray<T> &b, NDArray<T> &c) {
             // identify the correct output shape
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();

             // put result of addition into new output array
             NDArray<T> a{new_shape};
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> &b, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();
             NDArray<T> a{new_shape};
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> &b, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();

             NDArray<T> a{new_shape};
             a.Subtract(b, c);
             return a;
           })
      .def("__truediv__",
           [](NDArray<T> &b, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(b.shape(), c.shape(), new_shape))) throw py::index_error();

             NDArray<T> a{new_shape};
             a.Divide(b, c);
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
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Subtract(b, c);
             return a;
           })
      .def("__truediv__",
           [](NDArray<T> &b, T const &c) {
             NDArray<T> a(b.size());
             a.LazyReshape(b.shape());
             a.Divide(b, c);
             return a;
           })
      .def("__iadd__",
           [](NDArray<T> &a, NDArray<T> &c) {
             std::vector<std::size_t> new_shape;
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape))) throw py::index_error();
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
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape))) throw py::index_error();
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
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape))) throw py::index_error();
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
             if (!(ShapeFromBroadcast(a.shape(), c.shape(), new_shape))) throw py::index_error();
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
             if (idx >= s.size()) throw py::index_error();
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
      .def("max", [](NDArray<T> &a) { return a.Max(); })
      .def("max",
           [](NDArray<T> &a, std::size_t const axis) {
             if (axis >= a.shape().size()) throw py::index_error();
             return a.Max(axis);
           })
      .def("min", [](NDArray<T> const &a) { return a.Min(); })
      .def("min",
           [](NDArray<T> &a, std::size_t const axis) {
             if (axis >= a.shape().size()) throw py::index_error();
             return a.Min(axis);
           })
      .def("relu", [](NDArray<T> const &a, NDArray<T> &b) { return b.Relu(a); })
      .def("l2loss", [](NDArray<T> const &a) { return a.L2Loss(); })
      .def("sign", [](NDArray<T> const &a, NDArray<T> &b) { return b.Sign(a); })
      .def("reshape",
           [](NDArray<T> &a, std::vector<std::size_t> b) {
             if (!(a.CanReshape(b)))
             {
               py::print("cannot reshape array of size (", a.size(), ") into shape of(", a.shape(),
                         ")");
               throw py::value_error();
             }
             a.Reshape(b);
             return;
           })
      .def("shape", [](NDArray<T> &a) { return a.shape(); })
      .def_static("Zeros", &NDArray<T>::Zeroes)
      .def("abs", &NDArray<T>::Abs)
      .def("exp", &NDArray<T>::Exp)
      .def("exp2", &NDArray<T>::Exp2)
      .def("expm1", &NDArray<T>::Expm1)
      .def("log", &NDArray<T>::Log)
      .def("log10", &NDArray<T>::Log10)
      .def("log2", &NDArray<T>::Log2)
      .def("log1p", &NDArray<T>::Log1p)
      .def("sqrt", &NDArray<T>::Sqrt)
      .def("cbrt", &NDArray<T>::Cbrt)
      .def("sin", &NDArray<T>::Sin)
      .def("cos", &NDArray<T>::Cos)
      .def("tan", &NDArray<T>::Tan)
      .def("asin", &NDArray<T>::Asin)
      .def("acos", &NDArray<T>::Acos)
      .def("atan", &NDArray<T>::Atan)
      .def("sinh", &NDArray<T>::Sinh)
      .def("cosh", &NDArray<T>::Cosh)
      .def("tanh", &NDArray<T>::Tanh)
      .def("asinh", &NDArray<T>::Asinh)
      .def("acosh", &NDArray<T>::Acosh)
      .def("atanh", &NDArray<T>::Atanh)
      .def("ceil", &NDArray<T>::Ceil)
      .def("floor", &NDArray<T>::Floor)
      .def("trunc", &NDArray<T>::Trunc)
      .def("round", &NDArray<T>::Round)
      .def("rint", &NDArray<T>::Rint)
      .def("isfinite", &NDArray<T>::Isfinite)
      .def("isinf", &NDArray<T>::Isinf)
      .def("isnan", &NDArray<T>::Isnan)

      // functions not implemented in numpy:
      //      .def("erf", &NDArray<T>::Erf)
      //      .def("erfc", &NDArray<T>::Erfc)
      //      .def("tgamma", &NDArray<T>::Tgamma)
      //      .def("lgamma", &NDArray<T>::Lgamma)

      // functions with different return types (seems obsolete for python):
      //      .def("lround", &NDArray<T>::Lround)
      //      .def("llround", &NDArray<T>::Llround)
      //      .def("nearbyint", &NDArray<T>::Nearbyint)
      //      .def("lrint", &NDArray<T>::Lrint)
      //      .def("llrint", &NDArray<T>::Llrint)

      .def("scatter", &NDArray<T>::Scatter)
      .def("gather", &NDArray<T>::Gather)
      .def("FromNumpy",
           [](NDArray<T> &s, py::array_t<T> arr) {
             auto buf        = arr.request();
             using size_type = typename NDArray<T>::size_type;

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

             std::size_t total_size = NDArray<T>::SizeFromShape(shape);

             // copy the data
             T *ptr = (T *)buf.ptr;
             s.Resize(total_size);
             if (s.CanReshape(shape))
             {
               s.Reshape(shape);
             }
             else
             {
               throw py::index_error();
             }

             NDArrayIterator<T, typename NDArray<T>::container_type> it(s);

             for (std::size_t j = 0; j < total_size; ++j)
             {
               // Computing numpy index
               std::size_t i   = 0;
               std::size_t pos = 0;
               for (i = 0; i < shape.size(); ++i)
               {
                 pos += stride[i] * index[i];
               }
               if ((pos >= total_size))
               {
                 throw py::index_error();
               }

               // Updating
               *it = ptr[pos];
               ++it;

               // Increamenting Numpy
               i = 0;
               ++index[i];
               while (index[i] >= shape[i])
               {
                 index[i] = 0;
                 ++i;
                 if (i >= shape.size())
                 {
                   break;
                 }
                 ++index[i];
               }
             }
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

        // copy the data
        T *                                                     ptr = (T *)buf.ptr;
        NDArrayIterator<T, typename NDArray<T>::container_type> it(s);

        for (std::size_t j = 0; j < s.size(); ++j)
        {
          // Computing numpy index
          std::size_t i   = 0;
          std::size_t pos = 0;
          for (i = 0; i < shape.size(); ++i)
          {
            pos += stride[i] * index[i];
          }

          // Updating
          ptr[pos] = *it;
          ++it;

          // Increamenting Numpy
          i = 0;
          ++index[i];
          while (index[i] >= shape[i])
          {
            index[i] = 0;
            ++i;
            if (i >= shape.size())
            {
              break;
            }
            ++index[i];
          }
        }

        return result;
      });
  //                  [](NDArray<T> &a, NDArray<T> &b)
  //                  {
  //                    a.Relu(b);
  //                    return a;
  //                  });
}

}  // namespace math
}  // namespace fetch
