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

#include "ml/layers/fully_connected.hpp"
#include "python/fetch_pybind.hpp"

namespace py = pybind11;

namespace fetch {
namespace ml {
namespace ops {

template <typename T>
void BuildFullyConnected(std::string const &custom_name, pybind11::module &module)
{
  py::class_<fetch::ml::layers::FullyConnected<fetch::math::Tensor<T>>>(module, custom_name.c_str())
      .def(py::init<std::size_t, std::size_t>())
      .def("Forward", &fetch::ml::layers::FullyConnected<fetch::math::Tensor<T>>::Forward)
      .def("Backward", &fetch::ml::layers::FullyConnected<fetch::math::Tensor<T>>::Backward);
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
