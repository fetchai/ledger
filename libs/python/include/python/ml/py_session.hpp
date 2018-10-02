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

//#include "ml/layers/layers.hpp"
#include "ml/ops/ops.hpp"
#include "ml/session.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>

namespace fetch {
namespace ml {

template <typename ArrayType, typename VariableType>
void BuildSession(std::string const &custom_name, pybind11::module &module)
{
  namespace py = pybind11;
  py::class_<SessionManager<ArrayType, VariableType>>(module, custom_name.c_str())
      .def(py::init<>())
      //      .def("RegisterVariable", [](SessionManager<ArrayType, VariableType> &sess,
      //                                  layers::Layer<ArrayType> &a) { return
      //                                  sess.RegisterVariable(a); })
      .def("BackwardGraph", [](SessionManager<ArrayType, VariableType> &sess, VariableType &a) {
        return sess.BackwardGraph(a);
      });
}

}  // namespace ml
}  // namespace fetch
