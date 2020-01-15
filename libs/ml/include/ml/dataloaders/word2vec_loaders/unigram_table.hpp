#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "core/random/lcg.hpp"
#include "math/base_types.hpp"

#include <numeric>

namespace fetch {
namespace ml {

class UnigramTable
{
public:
  using SizeType = fetch::math::SizeType;
  explicit UnigramTable(std::vector<SizeType> const &frequencies = {}, SizeType size = 1e8);

  void ResetTable(std::vector<SizeType> const &count, SizeType size);
  bool Sample(SizeType &ret);
  bool SampleNegative(SizeType positive_index, SizeType &ret);
  bool SampleNegative(fetch::math::Tensor<SizeType> const &positive_indices, SizeType &ret);
  void ResetRNG();
  std::vector<SizeType> GetTable();

private:
  std::vector<SizeType>                      data_;
  fetch::random::LinearCongruentialGenerator rng_;
  SizeType                                   timeout_ = 100;
};

}  // namespace ml
}  // namespace fetch
