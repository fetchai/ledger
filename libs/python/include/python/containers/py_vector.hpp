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

#include "containers/vector.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace containers {

template <typename T>
void BuildVector(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Vector<T>>(module, custom_name.c_str())
      .def(py::init<>()) /* No constructors found */
      .def("Insert", &Vector<T>::Insert)
      .def("empty", &Vector<T>::empty)
      .def("capacity", &Vector<T>::capacity)
      .def("swap", &Vector<T>::swap)
      .def("Clear", &Vector<T>::Clear)
      .def("Back",
           (typename fetch::containers::Vector<T>::type & (Vector<T>::*)()) & Vector<T>::Back)
      .def("Back", (const typename fetch::containers::Vector<T>::type &(Vector<T>::*)() const) &
                       Vector<T>::Back)
      .def("Erase", &Vector<T>::Erase)
      .def("At", (typename fetch::containers::Vector<T>::type & (Vector<T>::*)(const std::size_t)) &
                     Vector<T>::At)
      .def("At", (const typename fetch::containers::Vector<T>::type &(
                     Vector<T>::*)(const std::size_t) const) &
                     Vector<T>::At)
      .def("operator[]",
           (typename fetch::containers::Vector<T>::type & (Vector<T>::*)(const std::size_t)) &
               Vector<T>::operator[])
      .def("operator[]", (const typename fetch::containers::Vector<T>::type &(
                             Vector<T>::*)(const std::size_t) const) &
                             Vector<T>::operator[])
      .def("PopBack", &Vector<T>::PopBack)
      .def("Front",
           (typename fetch::containers::Vector<T>::type & (Vector<T>::*)()) & Vector<T>::Front)
      .def("Front", (const typename fetch::containers::Vector<T>::type &(Vector<T>::*)() const) &
                        Vector<T>::Front)
      .def("size", &Vector<T>::size)
      .def("PushBack", &Vector<T>::PushBack)
      .def("Resize", &Vector<T>::Resize)
      .def("Reserve", &Vector<T>::Reserve);
}
};  // namespace containers
};  // namespace fetch
