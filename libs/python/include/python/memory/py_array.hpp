#ifndef LIBFETCHCORE_MEMORY_ARRAY_HPP
#define LIBFETCHCORE_MEMORY_ARRAY_HPP

#include "python/fetch_pybind.hpp"
#include "vectorise/memory/array.hpp"

namespace fetch {
namespace memory {

template <typename T>
void BuildArray(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Array<T>>(module, custom_name.c_str())
      .def(py::init<const std::size_t &>())
      .def(py::init<>())
      .def(py::init<const Array<T> &>())
      //    .def(py::init< Array<T> && >())
      .def("simd_size", &Array<T>::simd_size)
      .def("begin", &Array<T>::begin)
      .def("Set", &Array<T>::Set)
      .def("end", &Array<T>::end)
      .def("rend", &Array<T>::rend)
      .def("rbegin", &Array<T>::rbegin)
      .def("padded_size", &Array<T>::padded_size)
      .def("At", (T & (Array<T>::*)(const std::size_t &)) & Array<T>::At)
      .def("At",
           (const T &(Array<T>::*)(const std::size_t &)const) & Array<T>::At)
      .def("operator[]",
           (T & (Array<T>::*)(const std::size_t &)) & Array<T>::operator[])
      .def("operator[]", (const T &(Array<T>::*)(const std::size_t &)const) &
                             Array<T>::operator[])
      //    .def(py::self = py::self )
      .def("pointer", &Array<T>::pointer)
      .def("size", &Array<T>::size);
}
};  // namespace memory
};  // namespace fetch

#endif
