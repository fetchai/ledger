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

#include "commandline/parameter_parser.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace commandline {

void BuildParamsParser(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<ParamsParser>(module, "ParamsParser")
      .def(py::init<>()) /* No constructors found */
      .def("Parse", &ParamsParser::Parse)
      .def("arg_size", &ParamsParser::arg_size)
      .def("GetArg",
           (std::string(ParamsParser::*)(const std::size_t &) const) & ParamsParser::GetArg)
      .def("GetArg",
           (std::string(ParamsParser::*)(const std::size_t &, const std::string &) const) &
               ParamsParser::GetArg);
  //    .def("GetParam", &ParamsParser::GetParam);
}
};  // namespace commandline
};  // namespace fetch
