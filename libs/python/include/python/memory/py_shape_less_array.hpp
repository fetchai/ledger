#pragma once

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
      .def("Copy", &ShapeLessArray<T>::Copy)

      .def("InlineAdd", (ShapeLessArray<T> &
                         (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, memory::Range const &)) &
                            ShapeLessArray<T>::InlineAdd)
      .def("InlineAdd", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
                            ShapeLessArray<T>::InlineAdd)
      .def("InlineAdd",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) & ShapeLessArray<T>::InlineAdd)
      .def("Add", (ShapeLessArray<T> &
                   (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, ShapeLessArray<T> const &)) &
                      ShapeLessArray<T>::Add)
      .def("Add", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                              ShapeLessArray<T> const &,
                                                              memory::Range const &)) &
                      ShapeLessArray<T>::Add)
      .def("Add",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const &)) &
               ShapeLessArray<T>::Add)

      .def("InlineSubtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                         memory::Range const &)) &
                                 ShapeLessArray<T>::InlineSubtract)
      .def("InlineSubtract",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
               ShapeLessArray<T>::InlineSubtract)
      .def("InlineSubtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) &
                                 ShapeLessArray<T>::InlineSubtract)
      .def("Subtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                   ShapeLessArray<T> const &,
                                                                   memory::Range const &)) &
                           ShapeLessArray<T>::Subtract)
      .def("Subtract", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                   ShapeLessArray<T> const &)) &
                           ShapeLessArray<T>::Subtract)
      .def("Subtract",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const &)) &
               ShapeLessArray<T>::Subtract)

      .def("InlineMultiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                         memory::Range const &)) &
                                 ShapeLessArray<T>::InlineMultiply)
      .def("InlineMultiply",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
               ShapeLessArray<T>::InlineMultiply)
      .def("InlineMultiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) &
                                 ShapeLessArray<T>::InlineMultiply)
      .def("Multiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                   ShapeLessArray<T> const &,
                                                                   memory::Range const &)) &
                           ShapeLessArray<T>::Multiply)
      .def("Multiply", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                   ShapeLessArray<T> const &)) &
                           ShapeLessArray<T>::Multiply)
      .def("Multiply",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const &)) &
               ShapeLessArray<T>::Multiply)

      .def("InlineDivide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                       memory::Range const &)) &
                               ShapeLessArray<T>::InlineDivide)
      .def("InlineDivide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &)) &
                               ShapeLessArray<T>::InlineDivide)
      .def("InlineDivide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(T const &)) &
                               ShapeLessArray<T>::InlineDivide)
      .def("Divide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                 ShapeLessArray<T> const &,
                                                                 memory::Range const &)) &
                         ShapeLessArray<T>::Divide)
      .def("Divide", (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &,
                                                                 ShapeLessArray<T> const &)) &
                         ShapeLessArray<T>::Divide)
      .def("Divide",
           (ShapeLessArray<T> & (ShapeLessArray<T>::*)(ShapeLessArray<T> const &, T const &)) &
               ShapeLessArray<T>::Divide)

      .def("__add__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](ShapeLessArray<T> const &b, ShapeLessArray<T> const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Divide(b, c);
             return a;
           })

      .def("__add__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Add(b, c);
             return a;
           })
      .def("__mul__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Multiply(b, c);
             return a;
           })
      .def("__sub__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Subtract(b, c);
             return a;
           })
      .def("__div__",
           [](ShapeLessArray<T> const &b, T const &c) {
             ShapeLessArray<T> a;
             a.LazyResize(b.size());
             a.Divide(b, c);
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

      .def_static("Zeros", &ShapeLessArray<T>::Zeros)
      .def_static("Arange", (ShapeLessArray<T>(*)(T const &, T const &, T const &)) &
                                ShapeLessArray<T>::Arange)
      .def_static("UniformRandom",
                  (ShapeLessArray<T>(*)(std::size_t const &)) & ShapeLessArray<T>::UniformRandom)
      .def_static("UniformRandomIntegers",
                  (ShapeLessArray<T>(*)(std::size_t const &, int64_t const &, int64_t const &)) &
                      ShapeLessArray<T>::UniformRandomIntegers)

      .def("AllClose", &ShapeLessArray<T>::AllClose)
      .def("Abs", &ShapeLessArray<T>::Abs)
      .def("Exp", &ShapeLessArray<T>::Exp)
      .def("Exp2", &ShapeLessArray<T>::Exp2)
      .def("Expm1", &ShapeLessArray<T>::Expm1)
      .def("Log", &ShapeLessArray<T>::Log)
      .def("Log10", &ShapeLessArray<T>::Log10)
      .def("Log2", &ShapeLessArray<T>::Log2)
      .def("Log1p", &ShapeLessArray<T>::Log1p)
      .def("Sqrt", &ShapeLessArray<T>::Sqrt)
      .def("Cbrt", &ShapeLessArray<T>::Cbrt)
      .def("Sin", &ShapeLessArray<T>::Sin)
      .def("Cos", &ShapeLessArray<T>::Cos)
      .def("Tan", &ShapeLessArray<T>::Tan)
      .def("Asin", &ShapeLessArray<T>::Asin)
      .def("Acos", &ShapeLessArray<T>::Acos)
      .def("Atan", &ShapeLessArray<T>::Atan)
      .def("Sinh", &ShapeLessArray<T>::Sinh)
      .def("Cosh", &ShapeLessArray<T>::Cosh)
      .def("Tanh", &ShapeLessArray<T>::Tanh)
      .def("Asinh", &ShapeLessArray<T>::Asinh)
      .def("Acosh", &ShapeLessArray<T>::Acosh)
      .def("Atanh", &ShapeLessArray<T>::Atanh)
      .def("Erf", &ShapeLessArray<T>::Erf)
      .def("Erfc", &ShapeLessArray<T>::Erfc)
      .def("Tgamma", &ShapeLessArray<T>::Tgamma)
      .def("Lgamma", &ShapeLessArray<T>::Lgamma)
      .def("Ceil", &ShapeLessArray<T>::Ceil)
      .def("Floor", &ShapeLessArray<T>::Floor)
      .def("Trunc", &ShapeLessArray<T>::Trunc)
      .def("Round", &ShapeLessArray<T>::Round)
      .def("Lround", &ShapeLessArray<T>::Lround)
      .def("Llround", &ShapeLessArray<T>::Llround)
      .def("Nearbyint", &ShapeLessArray<T>::Nearbyint)
      .def("Rint", &ShapeLessArray<T>::Rint)
      .def("Lrint", &ShapeLessArray<T>::Lrint)
      .def("Llrint", &ShapeLessArray<T>::Llrint)
      .def("Isfinite", &ShapeLessArray<T>::Isfinite)
      .def("Isinf", &ShapeLessArray<T>::Isinf)
      .def("Isnan", &ShapeLessArray<T>::Isnan)

      .def("Sort", (void (ShapeLessArray<T>::*)()) & ShapeLessArray<T>::Sort)
      .def("Max", &ShapeLessArray<T>::Max)
      .def("Min", &ShapeLessArray<T>::Min)
      .def("Mean", &ShapeLessArray<T>::Mean)
      .def("Product", &ShapeLessArray<T>::Product)
      .def("Sum", &ShapeLessArray<T>::Sum)
      .def("StandardDeviation", &ShapeLessArray<T>::StandardDeviation)
      .def("Variance", &ShapeLessArray<T>::Variance)
      .def("ApproxExp", &ShapeLessArray<T>::ApproxExp)
      .def("ApproxLog", &ShapeLessArray<T>::ApproxLog)
      .def("ApproxLogistic", &ShapeLessArray<T>::ApproxLogistic)
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
           [](ShapeLessArray<T> &s, py::array_t<double> const &arr) {
             std::cout << "WAS HERE -- NOT IMPLEMENTED" << std::endl;
           })

      .def("FromNumpy",
           [](ShapeLessArray<T> &s, py::array_t<T> arr) {
             auto                                          buf = arr.request();
             typedef typename ShapeLessArray<T>::size_type size_type;
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
};  // namespace math
};  // namespace fetch
