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

#include "chain/transaction.hpp"
#include "chain/transaction_layout.hpp"
#include "network/service/protocol.hpp"
#include "telemetry/telemetry.hpp"

#include <vector>

namespace fetch {
namespace ledger {

class TransactionStorageEngineInterface;

/**
 * The internal RPC protocol used by internally on a node to interface with a transaction
 * storage engine on a specified shard
 */
class TransactionStorageProtocol : public service::Protocol
{
public:
  using TxLayouts = std::vector<chain::TransactionLayout>;

  enum
  {
    ADD = 0,
    HAS,
    GET,
    GET_COUNT,
    GET_RECENT
  };

  TransactionStorageProtocol(TransactionStorageEngineInterface &storage, uint32_t lane);
  ~TransactionStorageProtocol() override = default;

private:
  static constexpr char const *LOGGING_NAME = "TxStorageProto";

  telemetry::CounterPtr   CreateCounter(char const *operation) const;
  telemetry::HistogramPtr CreateHistogram(char const *operation) const;

  void               Add(chain::Transaction const &tx);
  bool               Has(Digest const &tx_digest);
  chain::Transaction Get(Digest const &tx_digest);
  uint64_t           GetCount();
  TxLayouts          GetRecent(uint32_t max_to_poll);

  // config
  uint32_t const                     lane_;
  TransactionStorageEngineInterface &storage_;

  // telemetry
  telemetry::CounterPtr   add_total_;
  telemetry::CounterPtr   has_total_;
  telemetry::CounterPtr   get_total_;
  telemetry::CounterPtr   get_count_total_;
  telemetry::CounterPtr   get_recent_total_;
  telemetry::HistogramPtr add_durations_;
  telemetry::HistogramPtr has_durations_;
  telemetry::HistogramPtr get_durations_;
  telemetry::HistogramPtr get_count_durations_;
  telemetry::HistogramPtr get_recent_durations_;
};

}  // namespace ledger
}  // namespace fetch
