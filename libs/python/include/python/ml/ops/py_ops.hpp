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
#include "ml/variable.hpp"
#include "python/fetch_pybind.hpp"
#include <pybind11/stl.h>

namespace fetch {
namespace ml {
namespace ops {

template <typename ArrayType>
inline void BuildOps(std::string const &custom_name, pybind11::module &module)
{
  using SessionType  = fetch::ml::SessionManager<ArrayType, fetch::ml::Variable<ArrayType>>;
  using VariableType = fetch::ml::Variable<ArrayType>;

  namespace py = pybind11;
  module
      .def("Dot", [](VariableType &left, VariableType &right,
                     SessionType &sess) { return fetch::ml::ops::Dot(left, right, sess); })
      .def("Relu",
           [](VariableType &left, SessionType &sess) { return fetch::ml::ops::Relu(left, sess); })
      .def("Sigmoid", [](VariableType &left, SessionType & sess)
      {
        return fetch::ml::ops::Sigmoid(left, sess);
      })
      .def("Sum", [](VariableType &left, std::size_t const axis,
                     SessionType &sess) { return fetch::ml::ops::Sum(left, axis, sess); })
      .def("MeanSquareError",
           [](VariableType &left, VariableType &right, SessionType &sess) {
             return fetch::ml::ops::MeanSquareError(left, right, sess);
           })
      .def("CrossEntropyLoss", [](VariableType &left, VariableType &right, SessionType &sess) {
        return fetch::ml::ops::CrossEntropyLoss(left, right, sess);
      });

  //  .def(custom_name.c_str(), &WrapperStandardDeviation < RectangularArray < double >> )
  //      .def(custom_name.c_str(), &WrapperStandardDeviation < RectangularArray < float >> )
  //      .def(custom_name.c_str(), &WrapperStandardDeviation < ShapeLessArray < double >> )
  //      .def(custom_name.c_str(), &WrapperStandardDeviation < ShapeLessArray < float >> );
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch

// template <typename ArrayType>
// void BuildLayers(std::string const &custom_name, pybind11::module &module)
//{
//  using SessionType = fetch::ml::SessionManager<ArrayType, fetch::ml::Variable<ArrayType>>;
//  using SelfType    = fetch::ml::layers::Layer<ArrayType>;
//  using VariableType = fetch::ml::Variable<ArrayType>;
//
//  namespace py = pybind11;
//  py::class_<SelfType>(module, custom_name.c_str())
//      .def(py::init<SessionType &, std::size_t const &, std::size_t const &>())
//  .def(py::init<SessionType &, std::vector<std::size_t> const &>())
//
//  .def("Forward", [](SelfType &a, VariableType &activations) { return a.Forward(activations); })
//      .def("Step", [](SelfType &a, typename ArrayType::type &lr) { return a.Step(lr); })
//      .def("Weights", [](SelfType &a) { return a.weights(); })
//      .def("InputSize", [](SelfType &a) { return a.InputSize(); })
//      .def("OutputSize", [](SelfType &a) { return a.OutputSize(); });
//
//}
//
//}  // namespace layers
//}  // namespace ml
//}  // namespace fetch
