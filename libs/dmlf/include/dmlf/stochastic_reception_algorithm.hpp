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

#include <memory>

#include "dmlf/shuffle_algorithm_interface.hpp"

namespace fetch {
namespace dmlf {

class StochasticReceptionAlgorithm : public ShuffleAlgorithmInterface
{
public:
  explicit StochasticReceptionAlgorithm(double broadcast_proportion)
    : ShuffleAlgorithmInterface(0)
    , broadcast_proportion_(broadcast_proportion)
  {}

  ~StochasticReceptionAlgorithm() override = default;

  std::vector<std::size_t> GetNextOutputs() override
  {
    throw std::invalid_argument(
        "StochasticReceptionAlgorithm::GetNextOutputs should never be called.");
  }

  StochasticReceptionAlgorithm(StochasticReceptionAlgorithm const &other) = delete;
  StochasticReceptionAlgorithm &operator=(StochasticReceptionAlgorithm const &other) = delete;

  bool operator==(StochasticReceptionAlgorithm const &other) = delete;
  bool operator<(StochasticReceptionAlgorithm const &other)  = delete;

  double broadcast_proportion()
  {
    return broadcast_proportion_;
  }

protected:
private:
  double broadcast_proportion_;
};

}  // namespace dmlf
}  // namespace fetch
