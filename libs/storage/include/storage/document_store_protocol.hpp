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

#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "core/threading/synchronised_state.hpp"
#include "network/service/protocol.hpp"
#include "storage/document_store.hpp"
#include "storage/new_revertible_document_store.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <map>

namespace fetch {
namespace storage {

class RevertibleDocumentStoreProtocol : public fetch::service::Protocol
{
public:
  using connection_handle_type = network::AbstractConnection::connection_handle_type;
  using lane_type              = uint32_t;  // TODO(issue 12): Fetch from some other palce
  using CallContext            = service::CallContext;

  using Identifier = byte_array::ConstByteArray;

  static constexpr char const *LOGGING_NAME = "RevertibleDocumentStoreProtocol";

  enum
  {
    GET = 0,
    GET_OR_CREATE,
    LAZY_GET,
    SET,

    COMMIT,
    REVERT_TO_HASH,
    CURRENT_HASH,
    HASH_EXISTS,
    KEY_DUMP,
    RESET,

    LOCK = 20,
    UNLOCK,
    HAS_LOCK
  };

  explicit RevertibleDocumentStoreProtocol(NewRevertibleDocumentStore *doc_store, lane_type lane)
    : fetch::service::Protocol()
    , doc_store_(doc_store)
    , get_count_(CreateCounter(lane, "ledger_statedb_get_total", "The total no. get ops"))
    , get_create_count_(
          CreateCounter(lane, "ledger_statedb_get_create_total", "The total no. get/create ops"))
    , set_count_(CreateCounter(lane, "ledger_statedb_set_total", "The total no. set ops"))
    , commit_count_(CreateCounter(lane, "ledger_statedb_commit_total", "The total no. commit ops"))
    , revert_count_(CreateCounter(lane, "ledger_statedb_revert_total", "The total no. revert ops"))
    , current_hash_count_(CreateCounter(lane, "ledger_statedb_current_hash_total",
                                        "The total no. current_hash ops"))
    , hash_exists_count_(
          CreateCounter(lane, "ledger_statedb_hash_exist_total", "The total no. hash_exists ops"))
    , key_dump_count_(
          CreateCounter(lane, "ledger_statedb_key_dump_total", "The total no. key dump ops"))
    , reset_count_(CreateCounter(lane, "ledger_statedb_reset_total", "The total no. reset ops"))
    , lock_count_(CreateCounter(lane, "ledger_statedb_lock_total", "The total no. lock ops"))
    , unlock_count_(CreateCounter(lane, "ledger_statedb_unlock_total", "The total no. unlock ops"))
    , has_lock_count_(
          CreateCounter(lane, "ledger_statedb_has_lock_total", "The total no. has lock ops"))
    , get_durations_(CreateHistogram(lane, "ledger_statedb_get_request_seconds",
                                     "The histogram of get request durations"))
    , set_durations_(CreateHistogram(lane, "ledger_statedb_set_request_seconds",
                                     "The histogram of set request durations"))
    , lock_durations_(CreateHistogram(lane, "ledger_statedb_lock_request_seconds",
                                      "The histogram of lock request durations"))
    , unlock_durations_(CreateHistogram(lane, "ledger_statedb_unlock_request_seconds",
                                        "The histogram of unlock request durations"))
  {
    this->Expose(GET, this, &RevertibleDocumentStoreProtocol::Get);
    this->Expose(GET_OR_CREATE, this, &RevertibleDocumentStoreProtocol::GetOrCreate);
    this->Expose(SET, this, &RevertibleDocumentStoreProtocol::Set);

    // Functionality for hashing/state
    this->Expose(COMMIT, this, &RevertibleDocumentStoreProtocol::Commit);
    this->Expose(REVERT_TO_HASH, this, &RevertibleDocumentStoreProtocol::RevertToHash);
    this->Expose(CURRENT_HASH, this, &RevertibleDocumentStoreProtocol::CurrentHash);
    this->Expose(HASH_EXISTS, this, &RevertibleDocumentStoreProtocol::HashExists);
    this->Expose(KEY_DUMP, this, &RevertibleDocumentStoreProtocol::KeyDump);
    this->Expose(RESET, this, &RevertibleDocumentStoreProtocol::Reset);

    this->ExposeWithClientContext(LOCK, this, &RevertibleDocumentStoreProtocol::LockResource);
    this->ExposeWithClientContext(UNLOCK, this, &RevertibleDocumentStoreProtocol::UnlockResource);
    this->ExposeWithClientContext(HAS_LOCK, this, &RevertibleDocumentStoreProtocol::HasLock);
  }

  RevertibleDocumentStoreProtocol(NewRevertibleDocumentStore *doc_store, lane_type const &lane,
                                  lane_type const &maxlanes)
    : RevertibleDocumentStoreProtocol(doc_store, lane)
  {
    SetLaneLog2(maxlanes);
    assert(maxlanes == (1u << log2_lanes_));
  }

  bool HasLock(CallContext const *context)
  {
    if (!context)
    {
      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type(std::string("No context for HasLock.")));
    }

    bool has_lock = false;
    lock_status_.Apply([&context, &has_lock](LockStatus const &status) {
      has_lock = (status.is_locked && (status.client == context->sender_address));
    });

    has_lock_count_->increment();

    return has_lock;
  }

  bool LockResource(CallContext const *context)
  {
    telemetry::FunctionTimer const timer{*lock_durations_};

    if (!context)
    {
      // TODO(issue 11): set exception number
      throw serializers::SerializableException(0, byte_array_type{"No context for HasLock."});
    }

    // attempt to lock this shard
    bool success = false;
    lock_status_.Apply([&context, &success](LockStatus &status) {
      if (!status.is_locked)
      {
        status.is_locked = true;
        status.client    = context->sender_address;
        success          = true;
      }
    });

    // print an error message on failure
    if (!success)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Resource lock failed for: ", context->sender_address.ToBase64());
    }

    lock_count_->increment();

    return success;
  }

  bool UnlockResource(CallContext const *context)
  {
    telemetry::FunctionTimer const timer{*unlock_durations_};

    if (!context)
    {
      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type(std::string("No context for HasLock.")));
    }

    // attempt to unlock this shard
    bool success = false;
    lock_status_.Apply([&context, &success](LockStatus &status) {
      if (status.is_locked && (status.client == context->sender_address))
      {
        status.is_locked = false;
        status.client    = Identifier{};
        success          = true;
      }
    });

    if (!success)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Resource unlock failed for: ", context->sender_address.ToBase64());
    }

    unlock_count_->increment();

    return success;
  }

private:

  static telemetry::CounterPtr CreateCounter(lane_type lane, char const *name, char const *description)
  {
    return telemetry::Registry::Instance().CreateCounter(name, description, {{"lane", std::to_string(lane)}});
  }

  static telemetry::HistogramPtr CreateHistogram(lane_type lane, char const *name, char const *description)
  {
    return telemetry::Registry::Instance().CreateHistogram(
        {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
         0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
         0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
         0.001,    0.01,     0.1,      1,        10.,      100.},
        name, description, {{"lane", std::to_string(lane)}});
  }

  Document Get(ResourceID const &rid)
  {
    telemetry::FunctionTimer const timer{*get_durations_};

    auto const doc = doc_store_->Get(rid);
    get_count_->increment();
    return doc;
  }

  Document GetOrCreate(ResourceID const &rid)
  {
    telemetry::FunctionTimer const timer{*get_durations_};

    auto const doc = doc_store_->GetOrCreate(rid);
    get_create_count_->increment();
    return doc;
  }

  void Set(ResourceID const &rid, byte_array::ConstByteArray const &data)
  {
    telemetry::FunctionTimer const timer{*set_durations_};

    doc_store_->Set(rid, data);
    set_count_->increment();
  }

  void SetBulk(std::unordered_map<ResourceID, byte_array::ConstByteArray> const &updates)
  {
    telemetry::FunctionTimer const timer{*set_bulk_durations_};

    for (auto const &element : updates)
    {
      doc_store_->Set(element.first, element.second);
      set_count_->increment();
    }
  }

  NewRevertibleDocumentStore::Hash Commit()
  {
    auto const hash = doc_store_->Commit();
    commit_count_->increment();
    return hash;
  }

  bool RevertToHash(NewRevertibleDocumentStore::Hash const &hash)
  {
    auto const success = doc_store_->RevertToHash(hash);
    revert_count_->increment();
    return success;
  }

  NewRevertibleDocumentStore::Hash CurrentHash()
  {
    auto const hash = doc_store_->CurrentHash();
    current_hash_count_->increment();
    return hash;
  }

  bool HashExists(NewRevertibleDocumentStore::Hash const &hash)
  {
    auto const success = doc_store_->HashExists(hash);
    hash_exists_count_->increment();
    return success;
  }

  NewRevertibleDocumentStore::Keys KeyDump()
  {
    auto const keys = doc_store_->KeyDump();
    key_dump_count_->increment();
    return keys;
  }

  void Reset()
  {
    doc_store_->Reset();
    reset_count_->increment();
  }

  void SetLaneLog2(lane_type const &count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  NewRevertibleDocumentStore *doc_store_;

  uint32_t log2_lanes_ = 0;

  struct LockStatus
  {
    bool       is_locked{false};  ///< Flag to signal which client has locked the resource
    Identifier client;            ///< The identifier of the locking client
  };

  using SyncLockStatus = SynchronisedState<LockStatus>;

  SyncLockStatus lock_status_;

  telemetry::CounterPtr   get_count_;
  telemetry::CounterPtr   get_create_count_;
  telemetry::CounterPtr   set_count_;
  telemetry::CounterPtr   commit_count_;
  telemetry::CounterPtr   revert_count_;
  telemetry::CounterPtr   current_hash_count_;
  telemetry::CounterPtr   hash_exists_count_;
  telemetry::CounterPtr   key_dump_count_;
  telemetry::CounterPtr   reset_count_;
  telemetry::CounterPtr   lock_count_;
  telemetry::CounterPtr   unlock_count_;
  telemetry::CounterPtr   has_lock_count_;
  telemetry::HistogramPtr get_durations_;
  telemetry::HistogramPtr set_durations_;
  telemetry::HistogramPtr lock_durations_;
  telemetry::HistogramPtr unlock_durations_;
};

}  // namespace storage
}  // namespace fetch
