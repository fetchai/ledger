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

#include "miner.hpp"
#include "world.hpp"
#include "osm_handler.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

namespace mobi {

void run(std::size_t n_miners = 1, std::size_t mining_loops = 100)
{
  // set up the world - contains all data
  World world = World();

  // set up the solutions heap
  //  std::vector<cluster, score>
  //  auto solution_heap = std::make_heap();

  // instantiate miners
  std::vector<Miner> miners(n_miners);
  for (std::size_t i = 0; i < n_miners; ++i)
  {
    miners.emplace_back(Miner());
  }

  // mining loop
  for (std::size_t j = 0; j < mining_loops; ++j)
  {
    std::cout << "beginning_loop: " << j << std::endl;

    for (auto miner : miners)
    {
      //      auto solution = miner.AssignTargets(world.DataDump());
      //      solution_heap.push_heap(solution);
      miner.AssignTargets(world.GetVehicles(), world.GetTargets());
    }
  }
}

}  // namespace mobi

int main()
{

  mobi::run();

  return 0;
}