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

#include <cstddef>
#include <cstdint>
#include <memory>

namespace fetch {
namespace ledger {

class Block;

namespace testing {

class BlockGenerator
{
public:
  using BlockPtr = std::shared_ptr<Block>;

  BlockGenerator(std::size_t num_lanes, std::size_t num_slices);

  void Reset();

  BlockPtr Generate(BlockPtr const &from = BlockPtr{}, uint64_t weight = 1u);

  BlockPtr operator()(BlockPtr const &from = BlockPtr{}, uint64_t weight = 1u);

private:
  uint64_t    block_count_{0};
  std::size_t num_slices_{0};
  uint32_t    log2_num_lanes_{0};
};

}  // namespace testing
}  // namespace ledger
}  // namespace fetch
