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

#include "math/tensor.hpp"
#include "python/fetch_pybind.hpp"

namespace py = pybind11;

namespace fetch {
namespace math {

template <typename T>
void BuildTensor(std::string const &custom_name, pybind11::module &module)
{
  using ArrayType = typename fetch::math::Tensor<T>;

  py::class_<ArrayType, std::shared_ptr<ArrayType>>(module, custom_name.c_str())
      .def(py::init<std::vector<SizeType> const &>())
      .def("ToString", &ArrayType::ToString)
      .def("Size", &ArrayType::size)
      .def("Fill", [](ArrayType &a, T val) { return a.Fill(val); })
      .def("Slice", [](ArrayType &a, SizeType i) { return a.Slice(i); })
      .def("Slice", [](ArrayType const &a, SizeType i) { return a.Slice(i); })
      .def("At", [](ArrayType &a, SizeType i) { return a.At(i); })
      // TODO (private 877)
      .def("Set", (void (ArrayType::*)(SizeType const &, T)) & ArrayType::Set)
      .def("Set", (void (ArrayType::*)(SizeType const &, SizeType const &, T)) & ArrayType::Set)
      .def("Set", (void (ArrayType::*)(SizeType const &, SizeType const &, SizeType const &, T)) &
                      ArrayType::Set)
      .def("Set", (void (ArrayType::*)(SizeType const &, SizeType const &, SizeType const &,
                                       SizeType const &, T)) &
                      ArrayType::Set);
}

}  // namespace math
}  // namespace fetch
