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
#include "math/rectangular_array.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <typename T>
void BuildShapeLessArray(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ShapeLessArray<T>>(module, custom_name.c_str())
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def(py::init<const ShapeLessArray<T> &>())
      //    .def(py::self = py::self )
      .def("size", &ShapeLessArray<T>::size)
      //      .def("Copy", &ShapeLessArray<T>::Copy)
      .def("Copy",
           []() {
             ShapeLessArray<T> a;
             return a.Copy();
           })
      .def("Copy",
           [](ShapeLessArray<T> &b) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Copy(b);
             return a;
           })
      .def("InlineAdd", (ShapeLessArray<T> &
                         (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, memory::Range const &)) &
                            ShapeLessArray<T>::InlineAdd)
      .def("InlineAdd", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
                            ShapeLessArray<T>::InlineAdd)
      .def("InlineAdd",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) & ShapeLessArray<T>::InlineAdd)
      //      .def("Add", (ShapeLessArray<T> &
      //                   (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, ShapeLessArray<T> const
      //                   &)) &
      //                      ShapeLessArray<T>::Add)
      //      .def("Add", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
      //                                                              ShapeLessArray<T> const &,
      //                                                              memory::Range const &)) &
      //                      ShapeLessArray<T>::Add)
      //      .def("Add",
      //           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const
      //           &)) &
      //               ShapeLessArray<T>::Add)

      .def("InlineSubtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                         memory::Range const &)) &
                                 ShapeLessArray<T>::InlineSubtract)
      .def("InlineSubtract",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
               ShapeLessArray<T>::InlineSubtract)
      .def("InlineSubtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) &
                                 ShapeLessArray<T>::InlineSubtract)
      //      .def("Subtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const
      //      &,
      //                                                                   ShapeLessArray<T> const
      //                                                                   &, memory::Range const
      //                                                                   &)) &
      //                           ShapeLessArray<T>::Subtract)
      //      .def("Subtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const
      //      &,
      //                                                                   ShapeLessArray<T> const
      //                                                                   &)) &
      //                           ShapeLessArray<T>::Subtract)
      //      .def("Subtract",
      //           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const
      //           &)) &
      //               ShapeLessArray<T>::Subtract)

      .def("InlineMultiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                         memory::Range const &)) &
                                 ShapeLessArray<T>::InlineMultiply)
      .def("InlineMultiply",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
               ShapeLessArray<T>::InlineMultiply)
      .def("InlineMultiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) &
                                 ShapeLessArray<T>::InlineMultiply)
      //      .def("Multiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const
      //      &,
      //                                                                   ShapeLessArray<T> const
      //                                                                   &, memory::Range const
      //                                                                   &)) &
      //                           ShapeLessArray<T>::Multiply)
      //      .def("Multiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const
      //      &,
      //                                                                   ShapeLessArray<T> const
      //                                                                   &)) &
      //                           ShapeLessArray<T>::Multiply)
      //      .def("Multiply",
      //           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const
      //           &)) &
      //               ShapeLessArray<T>::Multiply)

      .def("InlineDivide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                       memory::Range const &)) &
                               ShapeLessArray<T>::InlineDivide)
      .def("InlineDivide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
                               ShapeLessArray<T>::InlineDivide)
      .def("InlineDivide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) &
                               ShapeLessArray<T>::InlineDivide)
      //      .def("Divide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
      //                                                                 ShapeLessArray<T> const &,
      //                                                                 memory::Range const &)) &
      //                         ShapeLessArray<T>::Divide)
      //      .def("Divide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
      //                                                                 ShapeLessArray<T> const &))
      //                                                                 &
      //                         ShapeLessArray<T>::Divide)
      //      .def("Divide",
      //           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const
      //           &)) &
      //               ShapeLessArray<T>::Divide)

      .def("__add__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Subtract(b, c, a);
             return a;
           })
      .def("__div__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Divide(b, c, a);
             return a;
           })

      .def("__add__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Add(b, c, a);
             return a;
           })
      .def("__mul__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Multiply(b, c, a);
             return a;
           })
      .def("__sub__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Subtract(b, c, a);
             return a;
           })
      .def("__div__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             Divide(b, c, a);
             return a;
           })
      .def("__iadd__",
           [](ShapeLessArray<T> &a, ShapeLessArray<T> const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](ShapeLessArray<T> &a, ShapeLessArray<T> const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](ShapeLessArray<T> &a, ShapeLessArray<T> const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](ShapeLessArray<T> &a, ShapeLessArray<T> const &c) {
             a.InlineDivide(c);
             return a;
           })
      .def("__iadd__",
           [](ShapeLessArray<T> &a, T const &c) {
             a.InlineAdd(c);
             return a;
           })
      .def("__imul__",
           [](ShapeLessArray<T> &a, T const &c) {
             a.InlineMultiply(c);
             return a;
           })
      .def("__isub__",
           [](ShapeLessArray<T> &a, T const &c) {
             a.InlineSubtract(c);
             return a;
           })
      .def("__idiv__",
           [](ShapeLessArray<T> &a, T const &c) {
             a.InlineDivide(c);
             return a;
           })

      .def_static("Zeros", &ShapeLessArray<T>::Zeroes)
      .def_static("Arange", (ShapeLessArray<T>(*)(T const &, T const &, T const &)) &
                                ShapeLessArray<T>::Arange)
      .def_static("UniformRandom",
                  (ShapeLessArray<T>(*)(std::size_t const &)) & ShapeLessArray<T>::UniformRandom)
      .def_static("UniformRandomIntegers",
                  (ShapeLessArray<T>(*)(std::size_t const &, int64_t const &, int64_t const &)) &
                      ShapeLessArray<T>::UniformRandomIntegers)

      .def("AllClose", &ShapeLessArray<T>::AllClose)

      // various free functions
      .def("Abs", [](ShapeLessArray<T> &a) { fetch::math::Abs(a); })
      .def("Exp", [](ShapeLessArray<T> &a) { fetch::math::Exp(a); })
      .def("Exp2", [](ShapeLessArray<T> &a) { fetch::math::Exp2(a); })
      .def("Expm1", [](ShapeLessArray<T> &a) { fetch::math::Expm1(a); })
      .def("Log", [](ShapeLessArray<T> &a) { fetch::math::Log(a); })
      .def("Log10", [](ShapeLessArray<T> &a) { fetch::math::Log10(a); })
      .def("Log2", [](ShapeLessArray<T> &a) { fetch::math::Log2(a); })
      .def("Log1p", [](ShapeLessArray<T> &a) { fetch::math::Log1p(a); })
      .def("Sqrt", [](ShapeLessArray<T> &a) { fetch::math::Sqrt(a); })
      .def("Cbrt", [](ShapeLessArray<T> &a) { fetch::math::Cbrt(a); })
      .def("Sin", [](ShapeLessArray<T> &a) { fetch::math::Sin(a); })
      .def("Cos", [](ShapeLessArray<T> &a) { fetch::math::Cos(a); })
      .def("Tan", [](ShapeLessArray<T> &a) { fetch::math::Tan(a); })
      .def("Asin", [](ShapeLessArray<T> &a) { fetch::math::Asin(a); })
      .def("Acos", [](ShapeLessArray<T> &a) { fetch::math::Acos(a); })
      .def("Atan", [](ShapeLessArray<T> &a) { fetch::math::Atan(a); })
      .def("Sinh", [](ShapeLessArray<T> &a) { fetch::math::Sinh(a); })
      .def("Cosh", [](ShapeLessArray<T> &a) { fetch::math::Cosh(a); })
      .def("Tanh", [](ShapeLessArray<T> &a) { fetch::math::Tanh(a); })
      .def("Asinh", [](ShapeLessArray<T> &a) { fetch::math::Asinh(a); })
      .def("Acosh", [](ShapeLessArray<T> &a) { fetch::math::Acosh(a); })
      .def("Atanh", [](ShapeLessArray<T> &a) { fetch::math::Atanh(a); })
      .def("Erf", [](ShapeLessArray<T> &a) { fetch::math::Erf(a); })
      .def("Erfc", [](ShapeLessArray<T> &a) { fetch::math::Erfc(a); })
      .def("Tgamma", [](ShapeLessArray<T> &a) { fetch::math::Tgamma(a); })
      .def("Lgamma", [](ShapeLessArray<T> &a) { fetch::math::Lgamma(a); })
      .def("Ceil", [](ShapeLessArray<T> &a) { fetch::math::Ceil(a); })
      .def("Floor", [](ShapeLessArray<T> &a) { fetch::math::Floor(a); })
      .def("Trunc", [](ShapeLessArray<T> &a) { fetch::math::Trunc(a); })
      .def("Round", [](ShapeLessArray<T> &a) { fetch::math::Round(a); })
      .def("Lround", [](ShapeLessArray<T> &a) { fetch::math::Lround(a); })
      .def("Llround", [](ShapeLessArray<T> &a) { fetch::math::Llround(a); })
      .def("Nearbyint", [](ShapeLessArray<T> &a) { fetch::math::Nearbyint(a); })
      .def("Rint", [](ShapeLessArray<T> &a) { fetch::math::Rint(a); })
      .def("Lrint", [](ShapeLessArray<T> &a) { fetch::math::Lrint(a); })
      .def("Llrint", [](ShapeLessArray<T> &a) { fetch::math::Llrint(a); })
      .def("Isfinite", [](ShapeLessArray<T> &a) { fetch::math::Isfinite(a); })
      .def("Isinf", [](ShapeLessArray<T> &a) { fetch::math::Isinf(a); })
      .def("Isnan", [](ShapeLessArray<T> &a) { fetch::math::Isnan(a); })
      .def("ApproxExp", [](ShapeLessArray<T> &a) { fetch::math::ApproxExp(a); })
      .def("ApproxLog", [](ShapeLessArray<T> &a) { fetch::math::ApproxLog(a); })
      .def("ApproxLogistic", [](ShapeLessArray<T> &a) { fetch::math::ApproxLogistic(a); })

      .def("Sort", (void (ShapeLessArray<T>::*)()) & ShapeLessArray<T>::Sort)
      .def("Max",
           [](ShapeLessArray<T> const &a) {
             T ret;
             Max(a, ret);
             return ret;
           })
      .def("Min",
           [](ShapeLessArray<T> const &a) {
             T ret;
             Min(a, ret);
             return ret;
           })
      //      .def("Mean", &ShapeLessArray<T>::Mean)
      .def("Product",
           [](ShapeLessArray<T> const &a) {
             T ret;
             Product(a, ret);
             return ret;
           })
      .def("Sum",
           [](ShapeLessArray<T> const &a) {
             T ret;
             Sum(a, ret);
             return ret;
           })
      //      .def("StandardDeviation", &ShapeLessArray<T>::StandardDeviation)
      //      .def("Variance", &ShapeLessArray<T>::Variance)
      //    .def("Round", &ShapeLessArray< T >::Round)
      .def("Fill", (void (ShapeLessArray<T>::*)(T const &)) & ShapeLessArray<T>::Fill)
      .def("Fill", (void (ShapeLessArray<T>::*)(T const &, memory::Range const &)) &
                       ShapeLessArray<T>::Fill)
      .def("At", (T & (ShapeLessArray<T>::*)(const typename ShapeLessArray<T>::size_type &)) &
                     ShapeLessArray<T>::At)
      .def("Reserve", &ShapeLessArray<T>::Reserve)
      .def("Resize", &ShapeLessArray<T>::Resize)
      .def("capacity", &ShapeLessArray<T>::capacity)
      .def("size", &ShapeLessArray<T>::size)
      .def("BooleanMask",
           [](ShapeLessArray<T> &a, ShapeLessArray<T> &mask) {
             ShapeLessArray<T> ret{std::size_t(Sum(mask))};
             fetch::math::BooleanMask(a, mask, ret);
             return ret;
           })
      .def("dynamic_stitch",
           [](ShapeLessArray<T> &a, std::vector<std::vector<std::size_t>> const &indices,
              std::vector<ShapeLessArray<T>> const &data) {
             fetch::math::DynamicStitch(a, indices, data);
             return a;
           })
      .def("__len__", [](const ShapeLessArray<T> &a) { return a.size(); })
      .def("__getitem__",
           [](const ShapeLessArray<T> &s, std::size_t i) {
             if (i >= s.size()) throw py::index_error();
             return s[i];
           })
      .def("__setitem__",
           [](ShapeLessArray<T> &s, std::size_t i, T const &v) {
             if (i >= s.size()) throw py::index_error();
             s[i] = v;
           })
      .def("__eq__",
           [](ShapeLessArray<T> &s, ShapeLessArray<T> const &other) {
             if (other.size() == s.size())
             {
               for (std::size_t i = 0; i < other.size(); ++i)
               {
                 if (other[i] != s[i]) return false;
               }
               return true;
             }
             else
               return false;
           })

      .def("FromNumpy",
           [](ShapeLessArray<T> &s, py::array_t<T> arr) {
             auto buf        = arr.request();
             using size_type = typename ShapeLessArray<T>::size_type;
             if (buf.ndim != 1) throw std::runtime_error("Dimension must be exactly one.");

             T *         ptr = (T *)buf.ptr;
             std::size_t idx = 0;
             s.Resize(size_type(buf.shape[0]));
             for (std::size_t i = 0; i < std::size_t(buf.shape[0]); ++i)
             {
               s[idx] = ptr[idx];
             }
           })
      .def("ToNumpy", [](ShapeLessArray<T> &s) {
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
