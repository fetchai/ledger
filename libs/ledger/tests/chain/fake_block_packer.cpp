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

#include "core/macros.hpp"
#include "ledger/chain/block.hpp"
#include "vectorise/platform.hpp"

#include "fake_block_packer.hpp"

void FakeBlockPacker::EnqueueTransaction(fetch::ledger::v2::Transaction const &)
{
}

void FakeBlockPacker::EnqueueTransaction(fetch::ledger::v2::TransactionLayout const &)
{
}

void FakeBlockPacker::GenerateBlock(fetch::ledger::Block &block, std::size_t num_lanes,
                                    std::size_t num_slices, fetch::ledger::MainChain const &chain)
{
  FETCH_UNUSED(chain);

  // populate the fields required
  block.body.log2_num_lanes = fetch::platform::ToLog2(static_cast<uint32_t>(num_lanes));
  block.body.slices.resize(num_slices);

  // cache the last block
  last_generated_block_ = block;
}

uint64_t FakeBlockPacker::GetBacklog() const
{
  return 0;
}

fetch::ledger::Block const &FakeBlockPacker::last_generated_block() const
{
  return last_generated_block_;
}
