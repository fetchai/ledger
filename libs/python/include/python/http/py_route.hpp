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

#include "http/route.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace http {

void BuildRoute(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<Route>(module, "Route")
      .def(py::init<>()) /* No constructors found */
      .def("Match", &Route::Match);
}
};  // namespace http
};  // namespace fetch
