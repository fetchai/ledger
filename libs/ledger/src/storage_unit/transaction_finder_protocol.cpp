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

#include "ledger/storage_unit/transaction_finder_protocol.hpp"

namespace fetch {
namespace ledger {

TxFinderProtocol::TxFinderProtocol()
{
  Expose(ISSUE_CALL_FOR_MISSING_TXS, this, &Self::IssueCallForMissingTxs);
}

bool TxFinderProtocol::Pop(Digest &digest)
{
  return resource_queue_.Pop(digest, std::chrono::milliseconds::zero());
}

void TxFinderProtocol::IssueCallForMissingTxs(DigestSet const &digests)
{
  for (auto const &digest : digests)
  {
    resource_queue_.Push(digest);
  }
}

}  // namespace ledger
}  // namespace fetch
