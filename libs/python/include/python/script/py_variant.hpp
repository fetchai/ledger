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

#include "script/variant.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace script {

void BuildVariant(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Variant>(module, "Variant")
      .def(py::init<>())
      .def(py::init<const int64_t &>())
      .def(py::init<const int32_t &>())
      .def(py::init<const int16_t &>())
      .def(py::init<const uint64_t &>())
      .def(py::init<const uint32_t &>())
      .def(py::init<const uint16_t &>())
      .def(py::init<const float &>())
      .def(py::init<const double &>())
      //    .def(py::init< const char * >())
      //    .def(py::init< const fetch::script::Variant::byte_array_type & >())
      //    .def(py::init< const char & >())
      //    .def(py::init< const std::initializer_list<Variant> & >())
      //    .def(py::init< const fetch::script::Variant & >())
      //    .def(py::self == char*() )
      //    .def(py::self == int() )
      //    .def(py::self == double() )
      //    .def(py::self = fetch::script::Variant() )
      //    .def(py::self = std::initializer_list<Variant>() )
      //    .def(py::self = std::nullptr_t*() )
      //    .def(py::self = bool() )
      //    .def(py::self = char() )
      //    .def(py::self = fetch::script::Variant::byte_array_type() )
      /*
      .def("as_byte_array", ( const fetch::script::Variant::byte_array_type &
      (Variant::*)() const ) &Variant::as_byte_array) .def("as_byte_array", (
      fetch::script::Variant::byte_array_type & (Variant::*)() )
      &Variant::as_byte_array)
      //    .def("is_null", &Variant::is_null)
      //    .def("as_array", ( fetch::memory::SharedArray<Variant>
      (Variant::*)() ) &Variant::as_array)
      //    .def("as_array", ( const fetch::script::Variant::variant_array_type
      & (Variant::*)() const ) &Variant::as_array) .def("as_bool", ( const bool
      & (Variant::*)() const ) &Variant::as_bool) .def("as_bool", ( bool &
      (Variant::*)() ) &Variant::as_bool) .def("as_int", ( const int64_t &
      (Variant::*)() const ) &Variant::as_int) .def("as_int", ( int64_t &
      (Variant::*)() ) &Variant::as_int) .def("operator[]", (
      fetch::script::Variant & (Variant::*)(const std::size_t &) )
      &Variant::operator[]) .def("operator[]", ( const fetch::script::Variant &
      (Variant::*)(const std::size_t &) const ) &Variant::operator[])
      .def("operator[]", ( fetch::script::Variant & (Variant::*)(const
      byte_array::BasicByteArray &) ) &Variant::operator[]) .def("operator[]", (
      const fetch::script::Variant & (Variant::*)(const
      byte_array::BasicByteArray &) const ) &Variant::operator[])
      .def("as_double", ( const double & (Variant::*)() const )
      &Variant::as_double) .def("as_double", ( double & (Variant::*)() )
      &Variant::as_double)
      //    .def("Copy", &Variant::Copy)
      .def("type", &Variant::type)
      */
      .def("size", &Variant::size);
}
};  // namespace script
};  // namespace fetch
