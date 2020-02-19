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

#include "transaction_finder_protocol.hpp"
#include "transaction_store_sync_protocol.hpp"

#include "core/future_timepoint.hpp"
#include "core/service_ids.hpp"
#include "core/state_machine.hpp"
#include "ledger/storage_unit/lane_controller.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/rpc/client.hpp"
#include "network/generics/promise_of.hpp"
#include "network/generics/requesting_queue.hpp"
#include "storage/resource_mapper.hpp"
#include "telemetry/telemetry.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace ledger {

namespace tx_sync {
enum class State
{
  INITIAL = 0,
  QUERY_OBJECT_COUNTS,
  RESOLVING_OBJECT_COUNTS,
  QUERY_SUBTREE,
  RESOLVING_SUBTREE,
  QUERY_OBJECTS,
  RESOLVING_OBJECTS,
  TRIM_CACHE
};
}  // namespace tx_sync

class TransactionStorageEngineInterface;

class TransactionStoreSyncService : public TransactionSink
{
public:
  using Address               = muddle::Address;
  using Uri                   = network::Uri;
  using Client                = muddle::rpc::Client;
  using ClientPtr             = std::shared_ptr<Client>;
  using FutureTimepoint       = core::FutureTimepoint;
  using RequestingObjectCount = network::RequestingQueueOf<Address, uint64_t>;
  using PromiseOfObjectCount  = network::PromiseOf<uint64_t>;
  using TxArray               = std::vector<chain::Transaction>;
  using RequestingTxList      = network::RequestingQueueOf<Address, TxArray>;
  using RequestingSubTreeList = network::RequestingQueueOf<uint64_t, TxArray>;
  using PromiseOfTxList       = network::PromiseOf<TxArray>;
  using ResourceID            = storage::ResourceID;
  using EventNewTransaction   = std::function<void(chain::Transaction const &)>;
  using TrimCacheCallback     = std::function<void()>;
  using State                 = tx_sync::State;
  using StateMachine          = core::StateMachine<State>;
  using LaneControllerPtr     = std::shared_ptr<LaneController>;
  using TxFinderProtocolPtr   = std::shared_ptr<TxFinderProtocol>;
  using MuddleEndpoint        = muddle::MuddleEndpoint;

  static constexpr char const *LOGGING_NAME = "TransactionStoreSyncService";
  static constexpr std::size_t MAX_OBJECT_COUNT_RESOLUTION_PER_CYCLE = 128;
  static constexpr std::size_t MAX_SUBTREE_RESOLUTION_PER_CYCLE      = 128;
  static constexpr std::size_t MAX_OBJECT_RESOLUTION_PER_CYCLE       = 128;
  // Limit the amount to be retrieved at once from the TxFinderProtocol
  static constexpr uint64_t TX_FINDER_PROTO_LIMIT = 1000;
  // Limit the amount a single rpc call will provide
  static constexpr uint64_t PULL_LIMIT = 10000;

  struct Config
  {
    uint32_t                  lane_id{0};
    std::size_t               verification_threads{1};
    std::chrono::milliseconds main_timeout{5000};
    std::chrono::milliseconds promise_wait_timeout{2000};
    std::chrono::milliseconds fetch_object_wait_duration{5000};
  };

  TransactionStoreSyncService(Config const &cfg, MuddleEndpoint &muddle,
                              TransactionStorageEngineInterface &store,
                              TxFinderProtocol *                 tx_finder_protocol,
                              TrimCacheCallback                  trim_cache_callback);
  ~TransactionStoreSyncService() override = default;

  void Start()
  {
    verifier_.Start();
  }

  void Stop()
  {
    verifier_.Stop();
  }

  // We need this for the testing.
  bool IsReady()
  {
    return is_ready_;
  }

  void Execute()
  {
    if (state_machine_->IsReadyToExecute())
    {
      state_machine_->Execute();
    }
  }

protected:
  void OnTransaction(TransactionPtr const &tx) override;

private:
  State OnInitial();
  State OnQueryObjectCounts();
  State OnResolvingObjectCounts();
  State OnQuerySubtree();
  State OnResolvingSubtree();
  State OnQueryObjects();
  State OnResolvingObjects();
  State OnTrimCache();

  TrimCacheCallback                  trim_cache_callback_;
  std::shared_ptr<StateMachine>      state_machine_;
  TxFinderProtocol *                 tx_finder_protocol_;
  Config const                       cfg_;
  MuddleEndpoint &                   muddle_;
  ClientPtr                          client_;
  TransactionStorageEngineInterface &store_;  ///< The pointer to the object store
  TransactionVerifier                verifier_;

  FutureTimepoint promise_wait_timeout_;
  FutureTimepoint fetch_object_wait_timeout_;

  RequestingObjectCount pending_object_count_;
  uint64_t              max_object_count_{};

  RequestingSubTreeList pending_subtree_;
  RequestingTxList      pending_objects_;

  std::queue<uint64_t>                                          roots_to_sync_;
  uint64_t                                                      root_size_ = 0;
  std::unordered_map<PromiseOfTxList::PromiseCounter, uint64_t> promise_id_to_roots_;

  std::atomic_bool is_ready_{false};

  // telemetry
  telemetry::CounterPtr         stored_transactions_;
  telemetry::CounterPtr         resolve_count_failures_;
  telemetry::CounterPtr         subtree_requests_total_;
  telemetry::CounterPtr         subtree_response_total_;
  telemetry::CounterPtr         subtree_failure_total_;
  telemetry::GaugePtr<uint64_t> current_tss_state_;
  telemetry::GaugePtr<uint64_t> current_tss_peers_;
};

}  // namespace ledger
}  // namespace fetch
