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

#include "math/rectangular_array.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <typename T>
void BuildRectangularArray(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<RectangularArray<T>, ShapeLessArray<T>>(module, custom_name.c_str())
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def(py::init<const std::size_t &, const std::size_t &>())
      //    .def(py::init< RectangularArray<T> && >())
      .def(py::init<const RectangularArray<T> &>())
      //    .def(py::self = py::self )

      .def("Save", &RectangularArray<T>::Save)
      .def("size", &RectangularArray<T>::size)
      .def("height", &RectangularArray<T>::height)
      .def("width", &RectangularArray<T>::width)
      .def("padded_height", &RectangularArray<T>::padded_height)
      .def("padded_width", &RectangularArray<T>::padded_width)

      .def_static("Zeros", &RectangularArray<T>::Zeros)
      .def_static("UniformRandom", &RectangularArray<T>::UniformRandom)

      .def("Copy",
           [](RectangularArray<T> const &other) {
             RectangularArray<T> ret;
             ret.Copy(other);

             return ret;
           })

      .def("Sort", &RectangularArray<T>::Sort)
      .def("Flatten", &RectangularArray<T>::Flatten)
      .def("Reshape",
           [](RectangularArray<T> &ret, std::size_t const &h, std::size_t const &w) {
             if ((h * w) != (ret.height() * ret.width()))
               throw std::length_error("size does not match new size");
             ret.Reshape(h, w);
           })
      .def("Resize",
           (void (RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &)) &
               RectangularArray<T>::Resize)
      .def("Resize",
           (void (RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &,
                                          const typename RectangularArray<T>::size_type &)) &
               RectangularArray<T>::Resize)
      .def("Rotate", (void (RectangularArray<T>::*)(const double &,
                                                    const typename RectangularArray<T>::type)) &
                         RectangularArray<T>::Rotate)
      .def("Rotate", (void (RectangularArray<T>::*)(const double &, const double &, const double &,
                                                    const typename RectangularArray<T>::type)) &
                         RectangularArray<T>::Rotate)
      //    .def(py::self != py::self )
      //    .def(py::self == py::self )
      .def("data", (typename RectangularArray<T>::container_type & (RectangularArray<T>::*)()) &
                       RectangularArray<T>::data)
      .def("data",
           (const typename RectangularArray<T>::container_type &(RectangularArray<T>::*)() const) &
               RectangularArray<T>::data)
      .def("Load", &RectangularArray<T>::Load)
      .def("Set",
           (const T &(RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &,
                                              const T &)) &
               RectangularArray<T>::Set)
      .def("Set",
           (const T &(RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &,
                                              const typename RectangularArray<T>::size_type &,
                                              const T &)) &
               RectangularArray<T>::Set)
      .def("Crop",
           [](RectangularArray<T> &ret, RectangularArray<T> const &A, std::size_t const &i,
              std::size_t const &h, std::size_t const &j, std::size_t const &w) {
             if ((i + h) > A.height()) throw py::index_error("height of matrix exceeded");
             if ((j + w) > A.width()) throw py::index_error("width of matrix exceeded");

             ret.Resize(h, w);
             ret.Crop(A, i, h, j, w);
           })

      .def("Column",
           [](RectangularArray<T> &ret, RectangularArray<T> const &A, std::size_t const &i) {
             if (i >= A.width()) throw py::index_error("height of matrix exceeded");
             ret.Resize(A.height(), 1);
             ret.Column(A, i);
           })

      .def("Row",
           [](RectangularArray<T> &ret, RectangularArray<T> const &A, std::size_t const &i) {
             if (i >= A.height()) throw py::index_error("width of matrix exceeded");
             ret.Resize(1, A.width());
             ret.Row(A, i);
           })

      .def("Column",
           [](RectangularArray<T> &ret, RectangularArray<T> const &A, memory::Range const &range) {
             if (range.from() >= range.to()) throw py::index_error("i must be smaller than j");
             if (range.to() >= A.width()) throw py::index_error("width of matrix exceeded");
             ret.Resize(A.height(), range.to() - range.from());
             ret.Column(A, range.ToTrivialRange(A.width()));
           })

      .def("Row",
           [](RectangularArray<T> &ret, RectangularArray<T> const &A, memory::Range const &range) {
             if (range.from() >= range.to()) throw py::index_error("i must be smaller than j");
             if (range.to() >= A.height()) throw py::index_error("width of matrix exceeded");

             ret.Resize(range.to() - range.from(), A.width());
             ret.Row(A, range.ToTrivialRange(A.height()));
           })

      .def("At",
           (const typename RectangularArray<T>::type &(
               RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &)const) &
               RectangularArray<T>::At)
      .def("At",
           (const typename RectangularArray<T>::type &(
               RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &,
                                       const typename RectangularArray<T>::size_type &)const) &
               RectangularArray<T>::At)
      .def("At", (T & (RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &,
                                               const typename RectangularArray<T>::size_type &)) &
                     RectangularArray<T>::At)
      .def("At", (T & (RectangularArray<T>::*)(const typename RectangularArray<T>::size_type &)) &
                     RectangularArray<T>::At)

      .def("__getitem__",
           [](const RectangularArray<T> &s, int a) {
             if (a < 0) a += int(s.size());
             std::size_t i = std::size_t(a);
             if (i >= s.size()) throw py::index_error();

             return s[i];
           })
      .def("__setitem__",
           [](RectangularArray<T> &s, int a, T const &v) {
             if (a < 0) a += int(s.size());
             std::size_t i = std::size_t(a);
             if (i >= s.size()) throw py::index_error();

             s[i] = v;
           })
      .def("__getitem__",
           [](const RectangularArray<T> &s, py::tuple index) {
             if (py::len(index) != 2) throw py::index_error();
             int a = index[0].cast<int>();
             int b = index[1].cast<int>();
             if (a < 0) a += int(s.height());
             if (b < 0) b += int(s.width());
             std::size_t i = std::size_t(a);
             std::size_t j = std::size_t(b);

             if (i >= s.height()) throw py::index_error();
             if (j >= s.width()) throw py::index_error();

             return s(i, j);
           })
      .def("__setitem__",
           [](RectangularArray<T> &s, py::tuple index, T const &v) {
             if (py::len(index) != 2) throw py::index_error();
             int a = index[0].cast<int>();
             int b = index[1].cast<int>();
             if (a < 0) a += int(s.height());
             if (b < 0) b += int(s.width());
             std::size_t i = std::size_t(a);
             std::size_t j = std::size_t(b);

             if (std::size_t(i) >= s.height()) throw py::index_error();
             if (std::size_t(j) >= s.width()) throw py::index_error();

             s(i, j) = v;
           })
      .def("__eq__", [](RectangularArray<T> &      s,
                        py::array_t<double> const &arr) { std::cout << "WAS HERE" << std::endl; })
      .def("FromNumpy",
           [](RectangularArray<T> &s, py::array_t<T> arr) {
             auto buf        = arr.request();
             using size_type = typename RectangularArray<T>::size_type;
             if (buf.ndim != 2) throw std::runtime_error("Dimension must be exactly two.");

             T *         ptr = (T *)buf.ptr;
             std::size_t idx = 0;
             s.Resize(size_type(buf.shape[0]), size_type(buf.shape[1]));
             for (std::size_t i = 0; i < std::size_t(buf.shape[0]); ++i)
             {
               for (std::size_t j = 0; j < std::size_t(buf.shape[1]); ++j)
               {
                 s[idx] = ptr[idx];
                 ++idx;
               }
             }
           })
      .def("ToNumpy", [](RectangularArray<T> &s) {
        auto result = py::array_t<T>({s.height(), s.width()});
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
