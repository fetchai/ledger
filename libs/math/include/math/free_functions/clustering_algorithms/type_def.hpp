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

//#include "core/vector.hpp"
//#include "math/free_functions/free_functions.hpp"
//#include "math/free_functions/metrics/metrics.hpp"
//#include "math/meta/math_type_traits.hpp"
//#include "random"

#include "math/tensor.hpp"
#include <cstddef>

//#include <set>

/**
 * assigns the absolute of x to this array
 * @param x
 */

namespace fetch {
namespace math {
namespace clustering {

using ClusteringType = Tensor<std::int64_t>;

enum class ClusteringAlgorithm
{
  KMeans,
  MeanShift
};

enum class InitMode
{
  KMeansPP = 0,  // kmeans++, a good default choice
  Forgy    = 1,  // Forgy, randomly initialize clusters to data points
  PrevK    = 2   // PrevK, use previous k_assignment to determine cluster centres
};

enum class KInferenceMode
{
  Off            = 0,
  NClusters      = 1,  // infer K by counting number of previously assigned clusters
  HighestCluster = 2   // infer K by using highest valued previous cluster assignment
};

} // clustering
} // math
} // fetch