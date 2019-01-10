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

//#include "ml/layers/layers.hpp"
//#include <iostream>
//#include <memory>
//#include <unordered_map>
//#include <unordered_set>
#include "auctions/auction.hpp"
#include "math/linalg/matrix.hpp"

namespace fetch {
namespace auctions {


// template <typename A, typename V = fetch::ml::Variable<A>>
class CombinatorialAuction : public Auction
{
public:
  CombinatorialAuction(BlockIdType start_block_id, BlockIdType end_block_id, std::size_t max_items)
    : Auction(start_block_id, end_block_id, max_items)
  {}


  /**
   * Agent adds a single bid for a single item
   * @param agent_id
   * @param bid
   * @param item_id
   * @return
   */
  ErrorCode AddSingleBid(ValueType bid, AgentIdType bidder, ItemIdType item_id, std::vector<ItemIdType> excluded_item_ids = {})
  {

    // check item exists in auction
    if (!ItemInAuction(item_id))
    {
      return ErrorCode::ITEM_NOT_LISTED;
    }
    else
    {

      // count previous bid on item made by this bidder
      std::size_t n_bids = GetBidsCount(bidder, item_id);
      if (n_bids < max_bids_)
      {

        // update bid
        if (n_bids == 0)
        {
          items_[item_id].bids.insert(std::pair<AgentIdType, ValueType>(bidder, bid));
        }
        else
        {
          items_[item_id].bids[bidder] = bid;
        }

        // update bid counter
        IncrementBidCount(bidder, item_id);

        return ErrorCode::SUCCESS;
      }
      else
      {
        return ErrorCode::TOO_MANY_BIDS;
      }
    }
  }

private:




  std::size_t n_items_ = items_.size();

  // binary matrix
  fetch::math::linalg::Matrix<std::uint32_t> coupling_matrix_({n_items_, n_items_});

  // coupling strengths matrix
  fetch::math::linalg::Matrix<ValueType> J_({n_items_, n_items_});

  // bids on binary vector
  fetch::math::ShapelessArray<std::uint32_t> bids_on_(n_items_);

  // bids on binary vector
  fetch::math::ShapelessArray<std::uint32_t> local_field_(n_items_);


  ValueType Total_Energy_;


  ////
  // E = Sum (J_ * coupling_matrix_[i, j]) + Sum (local_field * bids_on_);
  /////
  void CalculateEnergy()
  {
    // Calculate Coupling strength sums
    // TODO - presumably we actually only need to do half of this work since the matrix must be symmetric?
    Total_Energy_ = 0;
    ValueType h = 0;
    for (std::size_t j = 0; j < items_.size(); ++j)
    {
      for (std::size_t k = 0; k < items_.size(); ++k)
      {
        Total_Energy_ += (J_.At(j, k) * coupling_matrix_.At(j, k));
      }

      // Now add the local field strength for this bid
      Total_Energy_ += bids_on_.At(j) * local_field_.At(j);
    }
  }




};

}  // namespace auctions
}  // namespace fetch


//
//class SmartMarket(object):
//
//    def BuildGraph(self):
//        # Building graph|
//        self.local_fields = [ 0 for i in range(len(self.bids))]
//        self.couplings = [ [0]*len(self.bids) for i in range(len(self.bids))]
//        self.active = [random.randint(0,1) for i in range(len(self.bids)) ]
//
//        for i in range(len(self.bids)):
//            bid1 = self.bids[i]
//
//            e = bid1["price"]
//            for n in range(len(self.items)):
//                if n in bid1["items"]:
//                    e -= self.items[n]
//
//            self.local_fields[i] = e
//
//            for j in range(i+1,len(self.bids)):
//                bid2 = self.bids[j]
//
//                coupling = 0
//                if i in bid2["excludes"]:
//                    coupling = 10
//
//                for n in range(len(self.items)):
//                    if n in bid1["items"] and n in bid2["items"]:
//                        coupling += (bid1["price"] + bid2["price"])
//
//                self.couplings[i][j] = self.couplings[j][i] = -coupling
//
//    def TotalBenefit(self):
//        reward = 0
//        for i in range(len(self.bids)):
//            a1 = self.active[i]
//            reward += a1*self.local_fields[i]
//
//            for j in range(len(self.bids)):
//                a2 = self.active[j]
//                reward += a1*a2*self.couplings[i][j]
//
//        return reward
//
//    def SelectBid(self, bid):
//        for j in range(len(self.bids)):
//            if self.couplings[bid][j] != 0:
//                self.active[j] = 0
//
//        self.active[bid] = 1
//
//    def Mine(self, hash_value, runtime):
//        random.seed(hash_value)
//        self.BuildGraph()
//        beta_start = 0.01
//        beta_end = 1
//        db = (beta_end-beta_start)/runtime
//        beta = beta_start
//
//        rejected = 0
//        for s in range(runtime):
//            for i in range(len(self.bids)):
//
//                oldactive = copy.copy(self.active)
//                oldreward = self.TotalBenefit()
//
//                nn = random.randint(1, 3)
//                for gg in range(nn):
//                    n = random.randint(0, len(self.bids)-1)
//
//                    if self.active[n] == 1:
//                        self.active[n] = 0
//                    else:
//                        self.active[n] = 1
//
//                #self.SelectBid( n )
//                newreward = self.TotalBenefit()
//                de = oldreward - newreward
//
//                if random.random() < math.exp(-beta*de):
//                    pass
//                    #print(oldreward, " => ", newreward, ":",de,beta,  math.exp(-beta*de))
//                else:
//                    self.active = oldactive
//                    rejected += 1
//
//            beta += db
//        #print(rejected/ runtime/len(self.bids))