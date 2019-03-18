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

#include "core/fixed_point/fixed_point.hpp"
#include "python/fetch_pybind.hpp"

namespace py = pybind11;

namespace fetch {
namespace fixed_point {

template <size_t I, size_t F>
void BuildFixedPoint(std::string const &custom_name, pybind11::module &module)
{
  py::class_<fetch::fixed_point::FixedPoint<I, F>>(module, custom_name.c_str())
      .def(py::init<float>())
      .def("__str__",
           [](fetch::fixed_point::FixedPoint<I, F> &n) { return std::to_string(float(n)); });
}

}  // namespace fixed_point
}  // namespace fetch
