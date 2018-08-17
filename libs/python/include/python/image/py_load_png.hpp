#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "pythin/image/load_png.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace image {

void BuildFileReadErrorException(pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<FileReadErrorException, std::exception>(module, "FileReadErrorException")
      .def(py::init<const std::string &, const std::string &>())
      .def("what", &FileReadErrorException::what);
}
};  // namespace image
};  // namespace fetch
