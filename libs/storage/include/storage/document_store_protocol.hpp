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

#include "core/threading/synchronised_state.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "network/service/protocol.hpp"
#include "storage/document_store.hpp"
#include "storage/new_revertible_document_store.hpp"
#include "storage/revertible_document_store.hpp"

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

    LOCK = 20,
    UNLOCK,
    HAS_LOCK
  };

  explicit RevertibleDocumentStoreProtocol(NewRevertibleDocumentStore *doc_store)
    : fetch::service::Protocol()
    , doc_store_(doc_store)
  {
    this->Expose(GET, doc_store, &NewRevertibleDocumentStore::Get);
    this->Expose(GET_OR_CREATE, doc_store, &NewRevertibleDocumentStore::GetOrCreate);
    this->Expose(SET, doc_store, &NewRevertibleDocumentStore::Set);

    // Functionality for hashing/state
    this->Expose(COMMIT, doc_store, &NewRevertibleDocumentStore::Commit);
    this->Expose(REVERT_TO_HASH, doc_store, &NewRevertibleDocumentStore::RevertToHash);
    this->Expose(CURRENT_HASH, doc_store, &NewRevertibleDocumentStore::CurrentHash);
    this->Expose(HASH_EXISTS, doc_store, &NewRevertibleDocumentStore::HashExists);

    this->ExposeWithClientContext(LOCK, this, &RevertibleDocumentStoreProtocol::LockResource);
    this->ExposeWithClientContext(UNLOCK, this, &RevertibleDocumentStoreProtocol::UnlockResource);
    this->ExposeWithClientContext(HAS_LOCK, this, &RevertibleDocumentStoreProtocol::HasLock);
  }

  RevertibleDocumentStoreProtocol(NewRevertibleDocumentStore *doc_store, lane_type const &lane,
                                  lane_type const &maxlanes)
    : RevertibleDocumentStoreProtocol(doc_store)
  {
    lane_assignment_ = lane;

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

    return has_lock;
  }

  bool LockResource(CallContext const *context)
  {
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
      FETCH_LOG_WARN(LOGGING_NAME, "Resource lock failed for: ", context->sender_address.ToBase64());
    }

    return success;
  }

  bool UnlockResource(CallContext const *context)
  {
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
      FETCH_LOG_WARN(LOGGING_NAME, "Resource unlock failed for: ", context->sender_address.ToBase64());
    }

    return success;
  }

private:
  Document GetLaneChecked(ResourceID const &rid)
  {
    if (lane_assignment_ != rid.lane(log2_lanes_))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Lane assignment is ", lane_assignment_, " vs ",
                      rid.lane(log2_lanes_));
      FETCH_LOG_DEBUG(LOGGING_NAME, "Address:", byte_array::ToHex(rid.id()));

      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type("Get: Resource located on other lane. TODO, set error number"));
    }

    return doc_store_->Get(rid);
  }

  Document GetOrCreateLaneChecked(ResourceID const &rid)
  {
    if (lane_assignment_ != rid.lane(log2_lanes_))
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Lane assignment is ", lane_assignment_, " vs ",
                      rid.lane(log2_lanes_));
      FETCH_LOG_DEBUG(LOGGING_NAME, "Address:", byte_array::ToHex(rid.id()));

      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type("GetOrCreate: Resource located on other lane. "
                             "TODO, set error number"));
    }

    return doc_store_->GetOrCreate(rid);
  }

  void SetLaneChecked(CallContext const *context, ResourceID const &rid,
                      byte_array::ConstByteArray const &value)
  {
    if (!context)
    {
      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type(std::string("No context for SetLaneChecked.")));
    }

    Identifier  identifier           = context->sender_address;
    std::string printable_identifier = static_cast<std::string>(ToBase64(identifier));

    if (lane_assignment_ != rid.lane(log2_lanes_))
    {
      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type(std::string("Set: Resource located on other lane:") + rid.ToString()));
    }

    // determine if this client has the lock on this resource
    if (!HasLock(context))
    {
      // TODO(issue 11): set exception number
      throw serializers::SerializableException(0, byte_array_type("This shard is locked by another client"));
    }

    // finally once all checks has passed we can set the value on the document store
    doc_store_->Set(rid, value);
  }

  void SetLaneLog2(lane_type const &count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  NewRevertibleDocumentStore *doc_store_;

  uint32_t log2_lanes_ = 0;

  uint32_t lane_assignment_ = 0;


  struct LockStatus
  {
    bool       is_locked{false}; ///< Flag to signal which client has locked the resource
    Identifier client;           ///< The identifier of the locking client
  };

  using SyncLockStatus = SynchronisedState<LockStatus>;

  SyncLockStatus lock_status_;
};

}  // namespace storage
}  // namespace fetch
