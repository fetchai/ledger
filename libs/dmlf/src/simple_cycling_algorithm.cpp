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

#include "dmlf/simple_cycling_algorithm.hpp"

namespace fetch {
namespace dmlf {

std::vector<std::size_t> SimpleCyclingAlgorithm::GetNextOutputs()
{
  std::vector<std::size_t> result(number_of_outputs_per_cycle);
  for (std::size_t i = 0; i < number_of_outputs_per_cycle; i++)
  {
    result[i] = next_output_index;
    next_output_index += 1;
    next_output_index %= GetCount();
  }
  return result;
}

SimpleCyclingAlgorithm::SimpleCyclingAlgorithm(std::size_t count,
                                               std::size_t number_of_outputs_per_cycle)
  : ShuffleAlgorithmInterface(count)
{
  this->next_output_index           = 0;
  this->number_of_outputs_per_cycle = std::min(number_of_outputs_per_cycle, GetCount());
}

}  // namespace dmlf
}  // namespace fetch
