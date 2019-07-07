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

#include "crypto/fnv.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace crypto {

void BuildFNV(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<FNV, fetch::crypto::HasherInterface>(module, "FNV")
      .def(py::init<>())
      .def("Reset", &FNV::Reset)
      .def("uint_digest", &FNV::uint_digest)
      .def("Update", &FNV::Update)
      .def("digest", &FNV::digest)
      .def("Final", &FNV::Final);
}
};  // namespace crypto
};  // namespace fetch
