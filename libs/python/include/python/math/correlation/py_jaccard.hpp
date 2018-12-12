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

#include "math/correlation/jaccard.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace correlation {

template <typename A>
inline typename A::Type WrapperJaccard(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return Jaccard(a, b);
}

inline void BuildJaccardCorrelation(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperJaccard<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperJaccard<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperJaccard<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperJaccard<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperJaccard<ShapelessArray<double>>)
      .def(custom_name.c_str(), &WrapperJaccard<ShapelessArray<float>>);
}

template <typename A>
inline typename A::Type WrapperGeneralisedJaccard(A const &a, A const &b)
{
  if (a.size() != b.size())
  {
    throw std::range_error("A and B must have same size");
  }

  return GeneralisedJaccard(a, b);
}

inline void BuildGeneralisedJaccardCorrelation(std::string const &custom_name,
                                               pybind11::module & module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperGeneralisedJaccard<Matrix<double>>)
      .def(custom_name.c_str(), &WrapperGeneralisedJaccard<Matrix<float>>)
      .def(custom_name.c_str(), &WrapperGeneralisedJaccard<RectangularArray<double>>)
      .def(custom_name.c_str(), &WrapperGeneralisedJaccard<RectangularArray<float>>)
      .def(custom_name.c_str(), &WrapperGeneralisedJaccard<ShapelessArray<double>>)
      .def(custom_name.c_str(), &WrapperGeneralisedJaccard<ShapelessArray<float>>);
}

}  // namespace correlation
}  // namespace math
}  // namespace fetch
