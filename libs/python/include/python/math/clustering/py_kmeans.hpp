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

#include "math/free_functions/clustering_algorithms/k_means.hpp"
#include "math/linalg/matrix.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace clustering {

template <typename A>
inline A WrapperKMeans(A const &a, std::size_t const &b)
{
  if (b > a.shape()[0])
  {
    throw std::range_error("cannot have more clusters than data points");
  }
  else if (b < 2)
  {
    throw std::range_error("cannot have fewer than 2 clusters");
  }

  return KMeans(a, b);
}

inline void BuildKMeansClustering(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math::linalg;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperKMeans<Matrix<double>>)
  .def(custom_name.c_str(), &WrapperKMeans<Matrix<float>>);
}

}  // namespace clustering
}  // namespace math
}  // namespace fetch
