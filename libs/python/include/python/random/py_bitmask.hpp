#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include "core/random/bitmask.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace random {

template <typename W, int B, bool MSBF>
void BuildBitMask(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<BitMask<W, B, MSBF>>(module, custom_name.c_str())
      .def(py::init<>()) /* No constructors found */
      .def("SetProbability", &BitMask<W, B, MSBF>::SetProbability);
}
};  // namespace random
};  // namespace fetch
