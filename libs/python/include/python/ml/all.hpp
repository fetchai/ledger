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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "python/ml/ops/py_fully_connected.hpp"
#include "python/ml/ops/py_mean_square_error.hpp"
#include "python/ml/ops/py_relu.hpp"
#include "python/ml/py_graph.hpp"

namespace py = pybind11;

namespace fetch {
namespace ml {

template <typename T>
void BuildMLLibrary(pybind11::module &module)
{
  fetch::ml::BuildGraph<T>("Graph", module);
  fetch::ml::ops::BuildRelu<T>("Relu", module);
  fetch::ml::ops::BuildFullyConnected<T>("FullyConnected", module);
  fetch::ml::ops::BuildMeanSquareError<T>("MeanSquareError", module);
}

}  // namespace ml
}  // namespace fetch
