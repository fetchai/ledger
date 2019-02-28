#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include "math/rectangular_array.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <typename T>
void BuildShapelessArray(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ShapelessArray<T>>(module, custom_name.c_str())
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def(py::init<const ShapelessArray<T> &>())
      //    .def(py::self = py::self )
      .def("size", &ShapelessArray<T>::size)
      //      .def("Copy", &ShapelessArray<T>::Copy)
      .def("Copy",
           []() {
             ShapelessArray<T> a;
             return a.Copy();
           })
      .def("Copy",
           [](ShapelessArray<T> &b) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             a.Copy(b);
             return a;
           })
      .def("InlineAdd", (ShapelessArray<T> &
                         (ShapelessArray<T>::*)(ShapelessArray<T> const &, memory::Range const &)) &
                            ShapelessArray<T>::InlineAdd)
      .def("InlineAdd", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &)) &
                            ShapelessArray<T>::InlineAdd)
      .def("InlineAdd",
           (ShapelessArray<T> & (ShapelessArray<T>::*)(T const &)) & ShapelessArray<T>::InlineAdd)
      //      .def("Add", (ShapelessArray<T> &
      //                   (ShapelessArray<T>::*)(ShapelessArray<T> const &, ShapelessArray<T> const
      //                   &)) &
      //                      ShapelessArray<T>::Add)
      //      .def("Add", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &,
      //                                                              ShapelessArray<T> const &,
      //                                                              memory::Range const &)) &
      //                      ShapelessArray<T>::Add)
      //      .def("Add",
      //           (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &, T const
      //           &)) &
      //               ShapelessArray<T>::Add)

      .def("InlineSubtract", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &,
                                                                         memory::Range const &)) &
                                 ShapelessArray<T>::InlineSubtract)
      .def("InlineSubtract",
           (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &)) &
               ShapelessArray<T>::InlineSubtract)
      .def("InlineSubtract", (ShapelessArray<T> & (ShapelessArray<T>::*)(T const &)) &
                                 ShapelessArray<T>::InlineSubtract)
      //      .def("Subtract", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const
      //      &,
      //                                                                   ShapelessArray<T> const
      //                                                                   &, memory::Range const
      //                                                                   &)) &
      //                           ShapelessArray<T>::Subtract)
      //      .def("Subtract", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const
      //      &,
      //                                                                   ShapelessArray<T> const
      //                                                                   &)) &
      //                           ShapelessArray<T>::Subtract)
      //      .def("Subtract",
      //           (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &, T const
      //           &)) &
      //               ShapelessArray<T>::Subtract)

      .def("InlineMultiply", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &,
                                                                         memory::Range const &)) &
                                 ShapelessArray<T>::InlineMultiply)
      .def("InlineMultiply",
           (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &)) &
               ShapelessArray<T>::InlineMultiply)
      .def("InlineMultiply", (ShapelessArray<T> & (ShapelessArray<T>::*)(T const &)) &
                                 ShapelessArray<T>::InlineMultiply)
      //      .def("Multiply", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const
      //      &,
      //                                                                   ShapelessArray<T> const
      //                                                                   &, memory::Range const
      //                                                                   &)) &
      //                           ShapelessArray<T>::Multiply)
      //      .def("Multiply", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const
      //      &,
      //                                                                   ShapelessArray<T> const
      //                                                                   &)) &
      //                           ShapelessArray<T>::Multiply)
      //      .def("Multiply",
      //           (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &, T const
      //           &)) &
      //               ShapelessArray<T>::Multiply)

      .def("InlineDivide", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &,
                                                                       memory::Range const &)) &
                               ShapelessArray<T>::InlineDivide)
      .def("InlineDivide", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &)) &
                               ShapelessArray<T>::InlineDivide)
      .def("InlineDivide", (ShapelessArray<T> & (ShapelessArray<T>::*)(T const &)) &
                               ShapelessArray<T>::InlineDivide)
      //      .def("Divide", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &,
      //                                                                 ShapelessArray<T> const &,
      //                                                                 memory::Range const &)) &
      //                         ShapelessArray<T>::Divide)
      //      .def("Divide", (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &,
      //                                                                 ShapelessArray<T> const &))
      //                                                                 &
      //                         ShapelessArray<T>::Divide)
      //      .def("Divide",
      //           (ShapelessArray<T> & (ShapelessArray<T>::*)(ShapelessArray<T> const &, T const
      //           &)) &
      //               ShapelessArray<T>::Divide)

      .def("__add__",
           [](ShapelessArray<T> const &b, ShapelessArray<T> const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](ShapelessArray<T> const &b, ShapelessArray<T> const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](ShapelessArray<T> const &b, ShapelessArray<T> const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Subtract(b, c, a);
             return a;
           })
      .def("__div__",
           [](ShapelessArray<T> const &b, ShapelessArray<T> const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Divide(b, c, a);
             return a;
           })

      .def("__add__",
           [](ShapelessArray<T> const &b, T const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](ShapelessArray<T> const &b, T const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](ShapelessArray<T> const &b, T const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Subtract(b, c, a);
             return a;
           })
      .def("__rsub__",
           [](ShapelessArray<T> const &b, T const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Subtract(c, b, a);
             return a;
           })
      .def("__div__",
           [](ShapelessArray<T> const &b, T const &c) {
             ShapelessArray<T> a;
             a.LazyResize(b.size());
             Divide(b, c, a);
             return a;
           })
      .def("__iadd__",
           [](ShapelessArray<T> &a, ShapelessArray<T> const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](ShapelessArray<T> &a, ShapelessArray<T> const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](ShapelessArray<T> &a, ShapelessArray<T> const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](ShapelessArray<T> &a, ShapelessArray<T> const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__iadd__",
           [](ShapelessArray<T> &a, T const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](ShapelessArray<T> &a, T const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](ShapelessArray<T> &a, T const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](ShapelessArray<T> &a, T const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__gt__",
           [](ShapelessArray<T> &a, T const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def_static("Zeros", &ShapelessArray<T>::Zeroes)
      .def_static("Arange",
                  [](int from, int to, int delta) {
                    if ((from > to) && (delta > 0))
                    {
                      throw std::runtime_error("invalid range specified");
                    }
                    else if ((from < to) && (delta < 0))
                    {
                      throw std::runtime_error("invalid range specified");
                    }
                    else
                    {
                      return ShapelessArray<T>::Arange(from, to, delta);
                    }
                  })
      .def_static("UniformRandom",
                  (ShapelessArray<T>(*)(std::size_t const &)) & ShapelessArray<T>::UniformRandom)
      .def_static("UniformRandomIntegers",
                  (ShapelessArray<T>(*)(std::size_t const &, int64_t const &, int64_t const &)) &
                      ShapelessArray<T>::UniformRandomIntegers)
      .def("AllClose", &ShapelessArray<T>::AllClose)

      // various free functions
      .def("Abs", [](ShapelessArray<T> &a) { fetch::math::Abs(a); })
      .def("Exp", [](ShapelessArray<T> &a) { fetch::math::Exp(a); })
      .def("Exp2", [](ShapelessArray<T> &a) { fetch::math::Exp2(a); })
      .def("Expm1", [](ShapelessArray<T> &a) { fetch::math::Expm1(a); })
      .def("Log", [](ShapelessArray<T> &a) { fetch::math::Log(a); })
      .def("Log10", [](ShapelessArray<T> &a) { fetch::math::Log10(a); })
      .def("Log2", [](ShapelessArray<T> &a) { fetch::math::Log2(a); })
      .def("Log1p", [](ShapelessArray<T> &a) { fetch::math::Log1p(a); })
      .def("Sqrt", [](ShapelessArray<T> &a) { fetch::math::Sqrt(a); })
      .def("Cbrt", [](ShapelessArray<T> &a) { fetch::math::Cbrt(a); })
      .def("Sin", [](ShapelessArray<T> &a) { fetch::math::Sin(a); })
      .def("Cos", [](ShapelessArray<T> &a) { fetch::math::Cos(a); })
      .def("Tan", [](ShapelessArray<T> &a) { fetch::math::Tan(a); })
      .def("Asin", [](ShapelessArray<T> &a) { fetch::math::Asin(a); })
      .def("Acos", [](ShapelessArray<T> &a) { fetch::math::Acos(a); })
      .def("Atan", [](ShapelessArray<T> &a) { fetch::math::Atan(a); })
      .def("Sinh", [](ShapelessArray<T> &a) { fetch::math::Sinh(a); })
      .def("Cosh", [](ShapelessArray<T> &a) { fetch::math::Cosh(a); })
      .def("Tanh", [](ShapelessArray<T> &a) { fetch::math::Tanh(a); })
      .def("Asinh", [](ShapelessArray<T> &a) { fetch::math::Asinh(a); })
      .def("Acosh", [](ShapelessArray<T> &a) { fetch::math::Acosh(a); })
      .def("Atanh", [](ShapelessArray<T> &a) { fetch::math::Atanh(a); })
      .def("Erf", [](ShapelessArray<T> &a) { fetch::math::Erf(a); })
      .def("Erfc", [](ShapelessArray<T> &a) { fetch::math::Erfc(a); })
      .def("Tgamma", [](ShapelessArray<T> &a) { fetch::math::Tgamma(a); })
      .def("Lgamma", [](ShapelessArray<T> &a) { fetch::math::Lgamma(a); })
      .def("Ceil", [](ShapelessArray<T> &a) { fetch::math::Ceil(a); })
      .def("Floor", [](ShapelessArray<T> &a) { fetch::math::Floor(a); })
      .def("Trunc", [](ShapelessArray<T> &a) { fetch::math::Trunc(a); })
      .def("Round", [](ShapelessArray<T> &a) { fetch::math::Round(a); })
      .def("Lround", [](ShapelessArray<T> &a) { fetch::math::Lround(a); })
      .def("Llround", [](ShapelessArray<T> &a) { fetch::math::Llround(a); })
      .def("Nearbyint", [](ShapelessArray<T> &a) { fetch::math::Nearbyint(a); })
      .def("Rint", [](ShapelessArray<T> &a) { fetch::math::Rint(a); })
      .def("Lrint", [](ShapelessArray<T> &a) { fetch::math::Lrint(a); })
      .def("Llrint", [](ShapelessArray<T> &a) { fetch::math::Llrint(a); })
      .def("Isfinite", [](ShapelessArray<T> &a) { fetch::math::Isfinite(a); })
      .def("Isinf", [](ShapelessArray<T> &a) { fetch::math::Isinf(a); })
      .def("Isnan", [](ShapelessArray<T> &a) { fetch::math::Isnan(a); })
      .def("ApproxExp", [](ShapelessArray<T> &a) { fetch::math::ApproxExp(a); })
      .def("ApproxLog", [](ShapelessArray<T> &a) { fetch::math::ApproxLog(a); })

      .def("Sort", (void (ShapelessArray<T>::*)()) & ShapelessArray<T>::Sort)
      .def("Max",
           [](ShapelessArray<T> const &a) {
             T ret;
             Max(a, ret);
             return ret;
           })
      .def("Min",
           [](ShapelessArray<T> const &a) {
             T ret;
             Min(a, ret);
             return ret;
           })
      .def("ArgMax",
           [](ShapelessArray<T> const &a) {
             T ret;
             ArgMax(a, ret);
             return ret;
           })
      //      .def("Mean", &ShapelessArray<T>::Mean)
      .def("Product",
           [](ShapelessArray<T> const &a) {
             T ret;
             Product(a, ret);
             return ret;
           })
      .def("Sum",
           [](ShapelessArray<T> const &a) {
             T ret;
             Sum(a, ret);
             return ret;
           })
      //      .def("StandardDeviation", &ShapelessArray<T>::StandardDeviation)
      //      .def("Variance", &ShapelessArray<T>::Variance)
      //    .def("Round", &ShapelessArray< T >::Round)
      .def("Fill", (void (ShapelessArray<T>::*)(T const &)) & ShapelessArray<T>::Fill)
      .def("Fill", (void (ShapelessArray<T>::*)(T const &, memory::Range const &)) &
                       ShapelessArray<T>::Fill)
      .def("At", (T & (ShapelessArray<T>::*)(const typename ShapelessArray<T>::SizeType &)) &
                     ShapelessArray<T>::At)
      .def("Reserve", &ShapelessArray<T>::Reserve)
      .def("Resize", &ShapelessArray<T>::Resize)
      .def("capacity", &ShapelessArray<T>::capacity)
      .def("size", &ShapelessArray<T>::size)
      .def("BooleanMask",
           [](ShapelessArray<T> &a, ShapelessArray<T> &mask) {
             ShapelessArray<T> ret{std::size_t(Sum(mask))};
             fetch::math::BooleanMask(a, mask, ret);
             return ret;
           })
      .def("dynamic_stitch",
           [](ShapelessArray<T> &a, ShapelessArray<T> const &indices,
              ShapelessArray<T> const &data) {
             fetch::math::DynamicStitch(a, indices, data);
             return a;
           })
      .def("concat",
           [](ShapelessArray<T> &input_array, std::vector<ShapelessArray<T>> concat_arrays) {
             fetch::math::Concat(input_array, concat_arrays);
             return input_array;
           })
      .def("__len__", [](const ShapelessArray<T> &a) { return a.size(); })
      .def("__getitem__",
           [](const ShapelessArray<T> &s, std::size_t i) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             return s[i];
           })
      .def("__setitem__",
           [](ShapelessArray<T> &s, std::size_t i, T const &v) {
             if (i >= s.size())
             {
               throw py::index_error();
             }
             s[i] = v;
           })
      .def("__eq__",
           [](ShapelessArray<T> &s, ShapelessArray<T> const &other) {
             if (other.size() == s.size())
             {
               for (std::size_t i = 0; i < other.size(); ++i)
               {
                 if (other[i] != s[i])
                 {
                   return false;
                 }
               }
               return true;
             }
             else
             {
               return false;
             }
           })

      .def("FromNumpy",
           [](ShapelessArray<T> &s, py::array_t<T> arr) {
             auto buf       = arr.request();
             using SizeType = typename ShapelessArray<T>::SizeType;
             if (buf.ndim != 1)
             {
               throw std::runtime_error("Dimension must be exactly one.");
             }

             T *         ptr = (T *)buf.ptr;
             std::size_t idx = 0;
             s.Resize(SizeType(buf.shape[0]));
             for (std::size_t i = 0; i < std::size_t(buf.shape[0]); ++i)
             {
               s[idx] = ptr[idx];
             }
           })
      .def("ToNumpy", [](ShapelessArray<T> &s) {
        auto result = py::array_t<T>({s.size()});
        auto buf    = result.request();

        T *ptr = (T *)buf.ptr;
        for (size_t i = 0; i < s.size(); ++i)
        {
          ptr[i] = s[i];
        }
        return result;
      });
}

}  // namespace math
}  // namespace fetch
