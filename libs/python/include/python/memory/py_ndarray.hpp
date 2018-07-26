#pragma once

#include "math/rectangular_array.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <typename T>
void BuildRectangularArray(std::string const &custom_name,
                           pybind11::module & module)
{

  namespace py = pybind11;
  py::class_<RectangularArray<T>, ShapeLessArray<T>>(module,
                                                     custom_name.c_str())
      .def(py::init<>())
      .def(py::init<const std::size_t &>())
      .def(py::init<const std::size_t &, const std::size_t &>())
      //    .def(py::init< RectangularArray<T> && >())
      .def(py::init<const RectangularArray<T> &>())
      //    .def(py::self = py::self )
      .def("height", &RectangularArray<T>::height)
      .def("Save", &RectangularArray<T>::Save)
      .def("size", &RectangularArray<T>::size)
      .def("width", &RectangularArray<T>::width)
      .def("Copy", &RectangularArray<T>::Copy)
      .def("Resize", (void (RectangularArray<T>::*)(
                         const typename RectangularArray<T>::size_type &)) &
                         RectangularArray<T>::Resize)
      .def("Resize", (void (RectangularArray<T>::*)(
                         const typename RectangularArray<T>::size_type &,
                         const typename RectangularArray<T>::size_type &)) &
                         RectangularArray<T>::Resize)
      .def("Rotate",
           (void (RectangularArray<T>::*)(
               const double &, const typename RectangularArray<T>::type)) &
               RectangularArray<T>::Rotate)
      .def("Rotate", (void (RectangularArray<T>::*)(
                         const double &, const double &, const double &,
                         const typename RectangularArray<T>::type)) &
                         RectangularArray<T>::Rotate)
      //    .def(py::self != py::self )
      //    .def(py::self == py::self )
      .def("data", (typename RectangularArray<T>::container_type &
                    (RectangularArray<T>::*)()) &
                       RectangularArray<T>::data)
      .def("data", (const typename RectangularArray<T>::container_type &(
                       RectangularArray<T>::*)() const) &
                       RectangularArray<T>::data)
      .def("Load", &RectangularArray<T>::Load)
      .def("Set",
           (const T &(RectangularArray<T>::
                          *)(const typename RectangularArray<T>::size_type &,
                             const T &)) &
               RectangularArray<T>::Set)
      .def("Set",
           (const T &(RectangularArray<T>::
                          *)(const typename RectangularArray<T>::size_type &,
                             const typename RectangularArray<T>::size_type &,
                             const T &)) &
               RectangularArray<T>::Set)
      .def("Crop", &RectangularArray<T>::Crop)
      .def("At",
           (const typename RectangularArray<T>::type &(
               RectangularArray<T>::
                   *)(const typename RectangularArray<T>::size_type &)const) &
               RectangularArray<T>::At)
      .def("At",
           (const typename RectangularArray<T>::type &(
               RectangularArray<T>::
                   *)(const typename RectangularArray<T>::size_type &,
                      const typename RectangularArray<T>::size_type &)const) &
               RectangularArray<T>::At)
      .def("At",
           (T & (RectangularArray<T>::
                     *)(const typename RectangularArray<T>::size_type &,
                        const typename RectangularArray<T>::size_type &)) &
               RectangularArray<T>::At)
      .def("At",
           (T & (RectangularArray<T>::
                     *)(const typename RectangularArray<T>::size_type &)) &
               RectangularArray<T>::At)

      .def("__getitem__",
           [](const RectangularArray<T> &s, std::size_t i) {
             if (i >= s.size()) throw py::index_error();
             return s[i];
           })
      .def("__setitem__",
           [](RectangularArray<T> &s, std::size_t i, T const &v) {
             if (i >= s.size()) throw py::index_error();
             s[i] = v;
           })
      .def("__getitem__",
           [](const RectangularArray<T> &s, py::tuple index) {
             if (py::len(index) != 2) throw py::index_error();
             std::size_t i = index[0].cast<std::size_t>();
             std::size_t j = index[1].cast<std::size_t>();
             if (std::size_t(i) >= s.height()) throw py::index_error();
             if (std::size_t(j) >= s.width()) throw py::index_error();

             return s(i, j);
           })
      .def("__setitem__",
           [](RectangularArray<T> &s, py::tuple index, T const &v) {
             if (py::len(index) != 2) throw py::index_error();
             std::size_t i = index[0].cast<std::size_t>();
             std::size_t j = index[1].cast<std::size_t>();
             if (std::size_t(i) >= s.height()) throw py::index_error();
             if (std::size_t(j) >= s.width()) throw py::index_error();

             s(i, j) = v;
           })
      .def("__eq__",
           [](RectangularArray<T> &s, py::array_t<double> const &arr) {
             std::cout << "WAS HERE" << std::endl;
           })
      .def("FromNumpy",
           [](RectangularArray<T> &s, py::array_t<T> arr) {
             auto buf = arr.request();
             typedef typename RectangularArray<T>::size_type size_type;
             if (buf.ndim != 2)
               throw std::runtime_error("Dimension must be exactly two.");

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
};  // namespace math
};  // namespace fetch

