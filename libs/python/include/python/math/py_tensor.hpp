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
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace fetch {
namespace math {

template <typename T>
void BuildTensor(std::string const &custom_name, pybind11::module &module)
{

  py::class_<fetch::math::Tensor<T>, std::shared_ptr<fetch::math::Tensor<T>>>(module,
                                                                              custom_name.c_str())
      .def(py::init<std::vector<size_t> const &>())
      .def("ToString", &fetch::math::Tensor<T>::ToString)
      .def("Size", &fetch::math::Tensor<T>::size)
      .def("Fill", &fetch::math::Tensor<T>::Fill)
      .def("Slice", &fetch::math::Tensor<T>::Slice)
      .def("At", [](fetch::math::Tensor<T> &a, size_t i) { return a.At(i); })
      .def("Set", (void (fetch::math::Tensor<T>::*)(std::vector<size_t> const &, T)) &
                      fetch::math::Tensor<T>::Set)
      .def("Set",
           [](fetch::math::Tensor<T> &a, std::vector<size_t> const &i, float v) { a.Set(i, T(v)); })
      .def("Set", (void (fetch::math::Tensor<T>::*)(size_t, T)) & fetch::math::Tensor<T>::Set);
}

}  // namespace math
}  // namespace fetch
