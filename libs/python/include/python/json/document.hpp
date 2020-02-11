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

#include "json/document.hpp"

#include "fetch_pybind.hpp"

namespace fetch {
namespace json {

void BuildJSONDocument(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<JSONDocument>(module, "JSONDocument")
      .def(py::init<>())
      .def("Parse", &JSONDocument::Parse)
      .def("operator[]", static_cast<script::Variant &(JSONDocument::*)(std::size_t const &)>(
                             &JSONDocument::operator[]))
      .def("operator[]",
           static_cast<script::Variant const &(JSONDocument::*)(std::size_t const &)const>(
               &JSONDocument::operator[]));
}

};  // namespace json
};  // namespace fetch
