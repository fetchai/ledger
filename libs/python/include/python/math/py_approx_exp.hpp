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

#include "math/approx_exp.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <uint8_t N, uint64_t C, bool O>
void BuildApproxExp(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<ApproxExp<N, C, O>>(module, custom_name.c_str())
      .def(py::init<const ApproxExp<N, C, O> &>())
      .def(py::init<>())
      //    .def(py::self = py::self )
      .def("SetCoefficient", &ApproxExp<N, C, O>::SetCoefficient);
}
};  // namespace math
};  // namespace fetch
