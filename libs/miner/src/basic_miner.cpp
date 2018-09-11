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

#include "miner/basic_miner.hpp"
#include "miner/resource_mapper.hpp"

namespace fetch {
namespace miner {
namespace {

bool SortByFee(BasicMiner::TransactionEntry const &a, BasicMiner::TransactionEntry const &b)
{
  return a.transaction.fee > b.transaction.fee;
}

} // namespace

BasicMiner::TransactionEntry::TransactionEntry(chain::TransactionSummary const &summary, uint32_t log2_num_lanes)
  : resources{}
  , transaction{summary}
{
  // update the resources array with the correct bits flags for the lanes
  for (auto const &resource : summary.resources)
  {
    // map the resource to a lane
    uint32_t const lane = MapResourceToLane(resource, summary.contract_name, log2_num_lanes);

    // update the bit flag
    resources.set(lane, 1);
  }
}

BasicMiner::BasicMiner(uint32_t log2_num_lanes)
  : log2_num_lanes_{log2_num_lanes}
{
  assert(log2_num_lanes <= LOG2_MAX_NUM_LANES);
}

BasicMiner::~BasicMiner()
{

}

void BasicMiner::EnqueueTransaction(chain::TransactionSummary const &tx)
{
  FETCH_LOCK(pending_lock_);
  pending_.emplace_back(tx, log2_num_lanes_);
}

void BasicMiner::GenerateBlock(chain::BlockBody &block, std::size_t num_lanes, std::size_t num_slices)
{
  assert(num_lanes < MAX_NUM_LANES);

  FETCH_LOCK(main_queue_lock_);

  // add the entire contents of the
  {
    FETCH_LOCK(pending_lock_);
    main_queue_.splice(main_queue_.end(), pending_);
  }

  // sort the queue
  main_queue_.sort(SortByFee);

  // resize the block correctly
  block.slices.clear();
  block.slices.resize(num_slices);
  for (std::size_t slice_idx = 0; slice_idx < num_slices; ++i)
  {
    BitVector state;

  }
}

} // namespace miner
} // namespace fetch
