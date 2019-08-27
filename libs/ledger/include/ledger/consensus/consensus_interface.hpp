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
#include <unordered_map>

namespace fetch {
namespace ledger {

class Block;

class ConsensusInterface
{
public:
  using NextBlockPtr = std::unique_ptr<Block>;

  enum class Status
  {
    YES,
    NO,
    UNKNOWN
  };

  ConsensusInterface()          = default;
  virtual ~ConsensusInterface() = default;

  // Let the consensus know which block you are on. Only valid
  // to update the current block incrementally forward but valid
  // to update backward any number
  virtual void UpdateCurrentBlock(Block const &current) = 0;

  // Populate the next block for packing and submission. Will return
  // an empty pointer if the miner should not emit a block
  virtual NextBlockPtr GenerateNextBlock() = 0;

  // Verify a block according to consensus requirements
  virtual Status ValidBlock(Block const &previous, Block const &current) = 0;

  // Refresh the consensus view on the main chain
  virtual void Refresh() = 0;
};

}  // namespace ledger
}  // namespace fetch
