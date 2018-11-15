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

#include "math/free_functions/statistics/standard_deviation.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace statistics {

template <typename A>
inline typename A::Type WrapperStandardDeviation(A const &a)
{
  return StandardDeviation(a);
}

inline void BuildStandardDeviationStatistics(std::string const &custom_name,
                                             pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperStandardDeviation<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperStandardDeviation<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperStandardDeviation<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperStandardDeviation<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperStandardDeviation<ShapeLessArray<double>>)
      .def(custom_name.c_str(), &WrapperStandardDeviation<ShapeLessArray<float>>);
}

}  // namespace statistics
}  // namespace math
}  // namespace fetch
