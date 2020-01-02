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

#include "core/containers/queue.hpp"
#include "core/digest.hpp"
#include "network/service/protocol.hpp"
#include "storage/resource_mapper.hpp"

#include <unordered_set>

namespace fetch {
namespace ledger {

/**
 * Create a transaction finder protocol
 */
class TxFinderProtocol : public fetch::service::Protocol
{
public:
  using ResourceIDs = std::unordered_set<storage::ResourceID>;

  enum
  {
    ISSUE_CALL_FOR_MISSING_TXS = 1
  };

  // Construction / Destruction
  TxFinderProtocol();
  TxFinderProtocol(TxFinderProtocol const &) = delete;
  TxFinderProtocol(TxFinderProtocol &&)      = delete;
  ~TxFinderProtocol() override               = default;

  // Operators
  TxFinderProtocol &operator=(TxFinderProtocol const &) = delete;
  TxFinderProtocol &operator=(TxFinderProtocol &&) = delete;

  /**
   * Retrieve Resource ID required by peer
   *
   * @param rid the Resource ID output
   * @return true if a Resource ID had been retrieved from the queue, false if the queue was empty
   */
  bool Pop(Digest &digest);

  /**
   * Store missing Tx IDs required by peer in a queue,
   *
   * @param rids The collection of resource IDs
   */
  void IssueCallForMissingTxs(DigestSet const &digests);

private:
  using Self = TxFinderProtocol;

  fetch::core::MPSCQueue<Digest, 1u << 15u> resource_queue_;
};

}  // namespace ledger
}  // namespace fetch
