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
#include "vectorise/memory/range.hpp"

namespace fetch {
namespace memory {

void BuildRange(std::string const &custom_name, pybind11::module &module)
{

  namespace py = pybind11;
  py::class_<Range>(module, custom_name.c_str())
      .def(py::init<std::size_t const &, std::size_t const &>())
      .def("from", &Range::from)
      .def("to", &Range::to);
}

}  // namespace memory
}  // namespace fetch
