#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
      .def(py::init<std::size_t const &>())
      .def(py::init<>())
      .def(py::init<SharedArray<T> const &>())
      .def("padded_size", &SharedArray<T>::padded_size)
      .def("At", static_cast<T &(SharedArray<T>::*)(std::size_t const &)>(&SharedArray<T>::At))
      .def("At",
           static_cast<T const &(SharedArray<T>::*)(std::size_t const &)const>(&SharedArray<T>::At))
      .def("operator[]",
           static_cast<T &(SharedArray<T>::*)(std::size_t const &)>(&SharedArray<T>::operator[]))
      .def("operator[]", static_cast<T const &(SharedArray<T>::*)(std::size_t const &)const>(
                             &SharedArray<T>::operator[]))
      .def("Copy", &SharedArray<T>::Copy)
      .def("size", [](SharedArray<T> const &o) { return o.size(); });
}

}  // namespace memory
}  // namespace fetch
