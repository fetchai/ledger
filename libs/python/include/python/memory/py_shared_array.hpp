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

#include "python/fetch_pybind.hpp"
#include "vectorise/memory/shared_array.hpp"

namespace fetch {
namespace memory {

template <typename T>
void BuildSharedArray(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<SharedArray<T>>(module, custom_name.c_str())
      .def(py::init<const std::size_t &>())
      .def(py::init<>())
      .def(py::init<const SharedArray<T> &>())
      //    .def(py::init< SharedArray<T> && >())
      .def("simd_size", &SharedArray<T>::simd_size)
      //    .def("begin", &SharedArray< T >::begin)
      //      .def("Set", &SharedArray<T>::Set)
      //    .def("end", &SharedArray< T >::end)
      //    .def("rend", &SharedArray< T >::rend)
      // g    .def("rbegin", &SharedArray< T >::rbegin)
      .def("padded_size", &SharedArray<T>::padded_size)
      .def("At", (T & (SharedArray<T>::*)(const std::size_t &)) & SharedArray<T>::At)
      .def("At", (const T &(SharedArray<T>::*)(const std::size_t &)const) & SharedArray<T>::At)
      .def("operator[]",
           (T & (SharedArray<T>::*)(const std::size_t &)) & SharedArray<T>::operator[])
      .def("operator[]",
           (const T &(SharedArray<T>::*)(const std::size_t &)const) & SharedArray<T>::operator[])
      //    .def(py::self = py::self )
      .def("Copy", &SharedArray<T>::Copy)
      .def("size", [](SharedArray<T> const &o) { return o.size(); });
}

}  // namespace memory
}  // namespace fetch
