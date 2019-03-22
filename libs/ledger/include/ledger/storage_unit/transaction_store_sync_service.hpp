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

#include "core/service_ids.hpp"
#include "ledger/storage_unit/lane_controller.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"
#include "network/generics/atomic_state_machine.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/promise_of.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "storage/resource_mapper.hpp"
#include "transaction_store_sync_protocol.hpp"

#include <algorithm>
#include <unordered_map>

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
}

class TransactionStoreSyncService : public network::AtomicStateMachine<tx_sync::State>,
                                    public VerifiedTransactionSink
{
public:
  using Muddle                = muddle::Muddle;
  using MuddlePtr             = std::shared_ptr<Muddle>;
  using Address               = Muddle::Address;
  using Uri                   = Muddle::Uri;
  using Client                = muddle::rpc::Client;
  using ClientPtr             = std::shared_ptr<Client>;
  using ObjectStore           = storage::TransientObjectStore<VerifiedTransaction>;
  using FutureTimepoint       = network::FutureTimepoint;
  using RequestingObjectCount = network::RequestingQueueOf<Address, uint64_t>;
  using PromiseOfObjectCount  = network::PromiseOf<uint64_t>;
  using TxList                = std::vector<UnverifiedTransaction>;
  using RequestingTxList      = network::RequestingQueueOf<Address, TxList>;
  using RequestingSubTreeList = network::RequestingQueueOf<uint64_t, TxList>;
  using PromiseOfTxList       = network::PromiseOf<TxList>;
  using ResourceID            = storage::ResourceID;
  using Mutex                 = mutex::Mutex;
  using EventNewTransaction   = std::function<void(VerifiedTransaction const &)>;
  using TrimCacheCallback     = std::function<void()>;
  using State                 = tx_sync::State;
  using ObjectStorePtr        = std::shared_ptr<ObjectStore>;
  using LaneControllerPtr     = std::shared_ptr<LaneController>;

  static constexpr char const *LOGGING_NAME = "TransactionStoreSyncService";
  static constexpr std::size_t MAX_OBJECT_COUNT_RESOLUTION_PER_CYCLE = 128;
  static constexpr std::size_t MAX_SUBTREE_RESOLUTION_PER_CYCLE      = 128;
  static constexpr std::size_t MAX_OBJECT_RESOLUTION_PER_CYCLE       = 128;
  static constexpr uint64_t PULL_LIMIT_ = 10000;  // Limit the amount a single rpc call will provide
  static const std::size_t  BATCH_SIZE;

  struct Config
  {
    uint32_t                  lane_id{0};
    std::size_t               verification_threads{1};
    std::chrono::milliseconds main_timeout{5000};
    std::chrono::milliseconds promise_wait_timeout{2000};
    std::chrono::milliseconds fetch_object_wait_duration{5000};
  };

  TransactionStoreSyncService(Config const &cfg, MuddlePtr muddle, ObjectStorePtr store);
  virtual ~TransactionStoreSyncService();

  void Start()
  {
    verifier_.Start();
  }

  void Stop()
  {
    verifier_.Stop();
  }

  void SetTrimCacheCallback(TrimCacheCallback const &callback)
  {
    trim_cache_callback_ = callback;
  }

  virtual bool PossibleNewState(State &current_state) override;

  // We need this for the testing.
  bool IsReady()
  {
    FETCH_LOCK(is_ready_mutex_);
    return is_ready_;
  }

protected:
  void OnTransaction(VerifiedTransaction const &tx) override;

  // Reverse bits in byte
  uint8_t Reverse(uint8_t c)
  {
    // https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
    return static_cast<uint8_t>(((c * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
  }

  void SetTimeOut()
  {
    if (timeout_set_)
    {
      return;
    }
    //FETCH_LOG_DEBUG(LOGGING_NAME, "Lane ", id_, ": ", "Timeout set ", time_duration_.count());
    timeout_.Set(cfg_.main_timeout);
    timeout_set_ = true;
  }

private:
  Config const        cfg_;
  MuddlePtr           muddle_;
  ClientPtr           client_;
  ObjectStorePtr      store_;  ///< The pointer to the object store
  TransactionVerifier verifier_;

  FutureTimepoint timeout_;
  FutureTimepoint promise_wait_timeout_;
  bool            timeout_set_ = false;
  FutureTimepoint fetch_object_wait_timeout_;

  RequestingObjectCount pending_object_count_;
  uint64_t              max_object_count_;

  RequestingSubTreeList pending_subtree_;
  RequestingTxList      pending_objects_;

  std::queue<uint8_t>                                          roots_to_sync_;
  uint64_t                                                     root_size_ = 0;
  std::unordered_map<PromiseOfTxList::PromiseCounter, uint8_t> promise_id_to_roots_;

  TrimCacheCallback trim_cache_callback_;

  Mutex mutex_{__LINE__, __FILE__};

  Mutex is_ready_mutex_{__LINE__, __FILE__};
  bool  is_ready_ = false;
};

}  // namespace ledger
}  // namespace fetch
