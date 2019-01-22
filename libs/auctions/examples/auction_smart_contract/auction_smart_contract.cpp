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

//#include "ledger/chaincode/dummy_contract.hpp"
#include "auction_smart_contract/auction_smart_contract.hpp"
#include <chrono>
#include <random>

using namespace fetch::auctions::example;

VickreyAuctionContract::VickreyAuctionContract(AgentId contract_owner_id, BlockId end_block,
                                               std::shared_ptr<MockLedger> ledger)
  : contract_owner_id_(contract_owner_id)
  , va_(end_block)
  , ledger_ptr_(ledger)
{}
