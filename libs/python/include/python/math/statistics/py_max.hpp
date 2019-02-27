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

#include "math/free_functions/matrix_operations/matrix_operations.hpp"
#include "math/tensor.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {

template <typename A>
inline void WrapperMax(A const &a, typename A::Type &ret)
{
  Max(a, ret);
}

inline void BuildMaxStatistics(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math;
  using namespace fetch::memory;

  namespace py = pybind11;
  //  module.def(custom_name.c_str(), &WrapperMax<ShapelessArray<double>>)
  //      .def(custom_name.c_str(), &WrapperMax<ShapelessArray<float>>)
  //      .def(custom_name.c_str(), &WrapperMax<RectangularArray<double>>)
  //      .def(custom_name.c_str(), &WrapperMax<RectangularArray<float>>)
  //      .def(custom_name.c_str(), &WrapperMax<Tensor<double>>)
  //      .def(custom_name.c_str(), &WrapperMax<Tensor<float>>);
  module.def(custom_name.c_str(), &WrapperMax<Tensor<double>>)
      .def(custom_name.c_str(), &WrapperMax<Tensor<float>>);
}

}  // namespace math
}  // namespace fetch
