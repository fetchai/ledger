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

#include "storage/versioned_random_access_stack.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace storage {

template <typename T, typename B>
void BuildVersionedRandomAccessStack(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<VersionedRandomAccessStack<T, B>, RandomAccessStack<T, B>>(module, custom_name)
      .def(py::init<>()) /* No constructors found */
      .def("Load", &VersionedRandomAccessStack<T, B>::Load)
      .def("Set", &VersionedRandomAccessStack<T, B>::Set)
      .def("Get", (typename RandomAccessStack<T>::type(VersionedRandomAccessStack<T, B>::*)(
                      const std::size_t &) const) &
                      VersionedRandomAccessStack<T, B>::Get)
      .def("Get", (void (VersionedRandomAccessStack<T, B>::*)(
                      const std::size_t &,
                      typename fetch::storage::VersionedRandomAccessStack<T, B>::type &) const) &
                      VersionedRandomAccessStack<T, B>::Get)
      .def("ResetBookmark", &VersionedRandomAccessStack<T, B>::ResetBookmark)
      .def("Clear", &VersionedRandomAccessStack<T, B>::Clear)
      .def("Revert", &VersionedRandomAccessStack<T, B>::Revert)
      .def("Pop", &VersionedRandomAccessStack<T, B>::Pop)
      .def("NextBookmark", &VersionedRandomAccessStack<T, B>::NextBookmark)
      .def("Swap", &VersionedRandomAccessStack<T, B>::Swap)
      .def("Commit", &VersionedRandomAccessStack<T, B>::Commit)
      .def("Push", &VersionedRandomAccessStack<T, B>::Push)
      .def("New", &VersionedRandomAccessStack<T, B>::New)
      .def("PreviousBookmark", &VersionedRandomAccessStack<T, B>::PreviousBookmark)
      .def("Top", &VersionedRandomAccessStack<T, B>::Top)
      .def("empty", &VersionedRandomAccessStack<T, B>::empty)
      .def("size", &VersionedRandomAccessStack<T, B>::size);
}
};  // namespace storage
};  // namespace fetch
