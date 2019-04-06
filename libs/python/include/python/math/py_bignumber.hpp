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

#include "math/bignumber.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

void BuildBigUnsigned(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<BigUnsigned, byte_array::ConstByteArray>(module, "BigUnsigned")
      .def(py::init<>())
      .def(py::init<const fetch::math::BigUnsigned &>())
      .def(py::init<const fetch::math::BigUnsigned::super_type &>())
      .def(py::init<const uint64_t &, std::size_t>())
      //    .def(py::self = fetch::math::BigUnsigned() )
      //    .def(py::self = byte_array::ConstByteArray() )
      //    .def(py::self = byte_array::ByteArray() )
      //    .def(py::self = byte_array::ConstByteArray() )
      .def(py::self < fetch::math::BigUnsigned())
      .def(py::self > fetch::math::BigUnsigned())
      .def("operator++", &BigUnsigned::operator++)
      .def("TrimmedSize", &BigUnsigned::TrimmedSize)
      .def("operator[]", &BigUnsigned::operator[])
      .def("operator<<=", &BigUnsigned::operator<<=);
}

}  // namespace math
}  // namespace fetch
