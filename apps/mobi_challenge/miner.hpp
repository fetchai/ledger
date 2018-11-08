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

#include "datum.hpp"
#include "target.hpp"
#include "vehicle.hpp"

#include "math/free_functions/clustering_algorithms/k_means.hpp"
#include "math/linalg/matrix.hpp"
//#include "memory/

#include <map>

namespace mobi {

class Miner
{
public:
  using VehicleType = std::map<std::size_t, Vehicle>;
  using TargetType  = std::map<std::size_t, Target>;

  using DataType      = double;
  using ContainerType = fetch::memory::SharedArray<DataType>;
  using MatrixType    = fetch::math::linalg::Matrix<DataType, ContainerType>;

  /**
   * Computes KMeans to identify which vehicle should go for which target
   * @param vehicles
   * @param targets
   * @return
   */
  std::map<std::size_t, std::vector<std::size_t>> AssignTargets(VehicleType const &vehicles,
                                                                TargetType const & targets)
  {

    std::size_t                           K = vehicles.size();
    std::vector<std::vector<std::size_t>> scheduled_tasks{};
    for (std::size_t k = 0; k < vehicles.size(); ++k)
    {
      scheduled_tasks.push_back({});
    }

    // convert maps of objects to matrices
    MatrixType  data{targets.size(), 2};  // longitude and latitude constitute 2 dimensions
    std::size_t t_count = 0;
    for (auto const &x : targets)
    {
      data.Set(t_count, 0, x.second.longitude);
      data.Set(t_count, 1, x.second.latitude);
      ++t_count;
    }

    // do clustering
    MatrixType clusters = fetch::math::clustering::KMeans(data, K);

    // convert return type
    std::map<std::size_t, std::vector<size_t>> return_map{};
    t_count = 0;
    for (auto target : targets)
    {
      scheduled_tasks[static_cast<std::size_t>(clusters.At(t_count, 0))].push_back(target.first);
      ++t_count;
    }

    for (std::size_t i = 0; i < K; ++i)
    {
      return_map.insert(std::pair<std::size_t, std::vector<std::size_t>>(i, scheduled_tasks[i]));
    }

    return return_map;
  }

private:
  void solve_problem()
  {}
};

}  // namespace mobi
