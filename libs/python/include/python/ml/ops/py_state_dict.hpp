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

#include "core/serializers/byte_array_buffer.hpp"
#include "vectorise/fixed_point/serializers.hpp"
#include "ml/ops/weights.hpp"
#include "ml/serializers/ml_types.hpp"
#include "python/fetch_pybind.hpp"

namespace py = pybind11;

namespace fetch {
namespace ml {
namespace ops {

template <typename T>
void BuildStateDict(std::string const &custom_name, pybind11::module &module)
{
  using ArrayType = fetch::math::Tensor<T>;
  py::class_<fetch::ml::StateDict<ArrayType>>(module, custom_name.c_str())
      .def(py::init<>())
      .def_readwrite("weights", &fetch::ml::StateDict<ArrayType>::weights_)
      .def_readwrite("dict", &fetch::ml::StateDict<ArrayType>::dict_)
      .def("Merge", &fetch::ml::StateDict<ArrayType>::Merge)
      .def("Serialize",
           [](fetch::ml::StateDict<ArrayType> &sd) {
             fetch::serializers::ByteArrayBuffer b;
             b << sd;
             return b;
           })
      .def("Deserialize", [](fetch::ml::StateDict<ArrayType> &   sd,
                             fetch::serializers::ByteArrayBuffer b) { b >> sd; });
}

}  // namespace ops
}  // namespace ml
}  // namespace fetch
