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

#include "math/clustering/k_means.hpp"
#include "math/tensor.hpp"
#include "python/fetch_pybind.hpp"

namespace fetch {
namespace math {
namespace clustering {

template <typename A>
inline fetch::math::clustering::ClusteringType WrapperKMeans(
    A const &data, std::size_t K, std::size_t r_seed, std::size_t max_loops,
    InitMode init_mode = InitMode::KMeansPP, std::size_t max_no_change_convergence = 10)
{
  if (K > data.shape()[0])
  {
    throw std::range_error("cannot have more clusters than data points");
  }
  else if (K < 2)
  {
    throw std::range_error("cannot have fewer than 2 clusters");
  }
  return KMeans(data, r_seed, K, max_loops, init_mode, max_no_change_convergence);
}

inline void BuildKMeansClustering(std::string const &custom_name, pybind11::module &module)
{
  using namespace fetch::math;
  using namespace fetch::memory;

  namespace py = pybind11;
  module.def(custom_name.c_str(), &WrapperKMeans<Tensor<double>>)
      .def(custom_name.c_str(), &WrapperKMeans<Tensor<float>>);
}

}  // namespace clustering
}  // namespace math
}  // namespace fetch
