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

#include "type_def.hpp"

namespace fetch {
namespace math {
namespace clustering {

template <typename ArrayType>
class Clusterer
{
    Clusterer(ArrayType const &data,
            std::size_t const &r_seed,
            ClusteringAlgorithm const algo,
            std::size_t const &n_clusters,
            std::size_t const &max_loops,
            InitMode init_mode,
            std::size_t max_no_change_convergence)
    : n_clusters_(n_clusters)
    , max_no_change_convergence_(std::move(max_no_change_convergence))
    , max_loops_(max_loops)
    , init_mode_(init_mode)
  {
  }


  ClusteringType



};

} // clustering
} // math
} // fetch