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

#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "python/fetch_pybind.hpp"

namespace py = pybind11;

namespace fetch {
namespace ml {
namespace ops {

template <typename T>
void BuildMeanSquareErrorLoss(std::string const &custom_name, pybind11::module &module)
{
  py::class_<fetch::ml::ops::MeanSquareErrorLoss<fetch::math::Tensor<T>>>(module,
                                                                          custom_name.c_str())
      .def(py::init<>())
      .def("Forward", &fetch::ml::ops::MeanSquareErrorLoss<fetch::math::Tensor<T>>::Forward)
      .def("Backward", &fetch::ml::ops::MeanSquareErrorLoss<fetch::math::Tensor<T>>::Backward);
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
