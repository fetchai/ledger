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

#include "ledger/storage_unit/transaction_finder_protocol.hpp"

namespace fetch {
namespace ledger {

TxFinderProtocol::TxFinderProtocol()
  : fetch::service::Protocol()
  , resource_queue_()
{
  this->Expose(ISSUE_CALL_FOR_MISSING_TXS, this, &Self::IssueCallForMissingTxs);
}

bool TxFinderProtocol::Pop(storage::ResourceID &rid)
{
  FETCH_LOG_DEBUG("FinderProto", "Popping resource ", rid.ToString());
  return resource_queue_.Pop(rid, std::chrono::milliseconds::zero());
}

void TxFinderProtocol::IssueCallForMissingTxs(ResourceIDs const &rids)
{
  for (auto const &rid : rids)
  {
    FETCH_LOG_DEBUG("FinderProto", "Stashing resource ", rid.ToString());
    resource_queue_.Push(rid);
  }
}

}  // namespace ledger
}  // namespace fetch
