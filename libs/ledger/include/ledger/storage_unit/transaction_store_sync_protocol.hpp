#pragma once
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

#include "core/logger.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"
#include "metrics/metrics.hpp"
#include "network/details/thread_pool.hpp"
#include "network/generics/milli_timer.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include "storage/transient_object_store.hpp"
#include "storage/resource_mapper.hpp"
#include "vectorise/platform.hpp"

#include <set>
#include <utility>
#include <vector>

namespace fetch {
namespace ledger {

class TransactionStoreSyncProtocol : public fetch::service::Protocol, public VerifiedTransactionSink
{
public:
  enum
  {
    OBJECT_COUNT  = 1,
    PULL_OBJECTS  = 2,
    PULL_SUBTREE  = 3,
    START_SYNC    = 4,
    FINISHED_SYNC = 5,
  };

  using Register = fetch::network::ConnectionRegister<LaneConnectivityDetails>;

  using UnverifiedTransaction = chain::UnverifiedTransaction;
  using VerifiedTransaction   = chain::VerifiedTransaction;
  using ObjectStore           = storage::TransientObjectStore<VerifiedTransaction>;
  using ProtocolId            = service::protocol_handler_type;
  using ThreadPool            = network::ThreadPool;
  using ServiceMap            = Register::service_map_type;

  static constexpr char const *LOGGING_NAME = "ObjectStoreSyncProtocol";

  // Construction / Destruction
  TransactionStoreSyncProtocol(ProtocolId const &p, Register r, ThreadPool tp, ObjectStore &store,
                               std::size_t verification_threads);
  TransactionStoreSyncProtocol(TransactionStoreSyncProtocol const &) = delete;
  TransactionStoreSyncProtocol(TransactionStoreSyncProtocol &&)      = delete;
  ~TransactionStoreSyncProtocol() override                           = default;

  void Start();
  void Stop();

  void OnNewTx(VerifiedTransaction const &o);

  // Operators
  TransactionStoreSyncProtocol &operator=(TransactionStoreSyncProtocol const &) = delete;
  TransactionStoreSyncProtocol &operator=(TransactionStoreSyncProtocol &&) = delete;

protected:

  void OnTransaction(chain::VerifiedTransaction const &tx) override;

private:
  static constexpr uint64_t PULL_LIMIT_ = 10000;  // Limit the amount a single rpc call will provide

  struct CachedObject
  {
    using Clock     = std::chrono::system_clock;
    using Timepoint = Clock::time_point;
    using HandleSet = std::unordered_set<uint64_t>;

    CachedObject(UnverifiedTransaction value)
      : data(std::move(value))
    {}

    CachedObject(UnverifiedTransaction &&value)
      : data(std::move(value))
    {}

    UnverifiedTransaction data;
    HandleSet             delivered_to;
    Timepoint             created{Clock::now()};
  };

  using Self   = TransactionStoreSyncProtocol;
  using Cache  = std::vector<CachedObject>;
  using TxList = std::vector<chain::UnverifiedTransaction>;

  uint64_t ObjectCount();
  TxList   PullObjects(uint64_t const &client_handle);

  void IdleUntilPeers();
  void SetupSync();
  void FetchObjectsFromPeers();

  void RealisePromises(std::size_t index = 0);
  void TrimCache();

  TxList PullSubtree(byte_array::ConstByteArray const &rid, uint64_t mask);

  void StartSync();
  bool FinishedSync();

  // Reverse bits in byte
  uint8_t Reverse(uint8_t c);

  // Create a stack of subtrees we want to sync. Push roots back onto this when the promise
  // fails. Completion when the stack is empty
  void SyncSubtree();
  void RealiseSubtreePromises();

  // TODO(issue 7): Make cache configurable
  static constexpr uint32_t MAX_CACHE_ELEMENTS    = 2000;  // really a "max"?
  static constexpr uint32_t MAX_CACHE_LIFETIME_MS = 20000;

  ProtocolId          protocol_;
  Register            register_;
  ThreadPool          thread_pool_;
  ObjectStore &       store_;  ///< The pointer to the object store
  TransactionVerifier verifier_;

  std::atomic<bool> running_{false};

  mutex::Mutex cache_mutex_{__LINE__, __FILE__};  ///< The mutex protecting cache_
  Cache        cache_;

  mutable mutex::Mutex          object_list_mutex_{__LINE__, __FILE__};
  std::vector<service::Promise> object_list_promises_;  // GUARDED_BY(object_list_mutex_);

  // Syncing with other peers on startup
  bool                                              needs_sync_ = true;
  std::vector<std::pair<uint8_t, service::Promise>> subtree_promises_;
  std::queue<uint8_t>                               roots_to_sync_;
  uint64_t                                          root_mask_ = 0;
};

// Reverse bits in byte
inline uint8_t TransactionStoreSyncProtocol::Reverse(uint8_t c)
{
  // https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
  return static_cast<uint8_t>(((c * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32);
}

}  // namespace ledger
}  // namespace fetch
