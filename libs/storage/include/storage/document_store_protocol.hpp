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

  bool HasLock(CallContext const *context, ResourceID const &rid)
  {
    Identifier  identifier           = context->sender_address;
    std::string printable_identifier = static_cast<std::string>(ToBase64(identifier));

    if (!context)
    {
      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type(std::string("No context for HasLock.")));
    }

    std::lock_guard<mutex::Mutex> lock(lock_mutex_);
    auto                          it = locks_.find(rid.id());
    if (it == locks_.end())
    {
      return false;
    }

    return (it->second == identifier);
  }

  bool LockResource(CallContext const *context, ResourceID const &rid)
  {
    if (!context)
    {
      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type(std::string("No context for HasLock.")));
    }

    Identifier  identifier           = context->sender_address;
    std::string printable_identifier = static_cast<std::string>(ToBase64(identifier));

    std::lock_guard<mutex::Mutex> lock(lock_mutex_);
    auto                          it = locks_.find(rid.id());
    if (it == locks_.end())
    {
      locks_[rid.id()] = identifier;
      return true;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "LockResource failed ", printable_identifier, " => ",
                    rid.ToString());
    return (it->second == identifier);
  }

  bool UnlockResource(CallContext const *context, ResourceID const &rid)
  {
    if (!context)
    {
      throw serializers::SerializableException(  // TODO(issue 11): set exception number
          0, byte_array_type(std::string("No context for HasLock.")));
    }

    Identifier  identifier           = context->sender_address;
    std::string printable_identifier = static_cast<std::string>(ToBase64(identifier));

    std::lock_guard<mutex::Mutex> lock(lock_mutex_);
    auto                          it = locks_.find(rid.id());
    if (it == locks_.end())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "UnlockResource not locked ", printable_identifier, " => ",
                      rid.ToString());
      return false;
    }

    if (it->second == identifier)
    {
      locks_.erase(it);
      return true;
    }

    FETCH_LOG_DEBUG(LOGGING_NAME, "LockResource denied ", printable_identifier, " => ",
                    rid.ToString());
    return false;
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
    {
      std::lock_guard<mutex::Mutex> lock(lock_mutex_);
      auto                          it = locks_.find(rid.id());

      if (it == locks_.end())
      {
        throw serializers::SerializableException(  // TODO(issue 11): set exception number
            0, byte_array_type(std::string("There is no lock for the resource:") + rid.ToString()));
      }
      if (it->second != identifier)
      {
        throw serializers::SerializableException(  // TODO(issue 11): set exception number
            0, byte_array_type(std::string("Client ") + printable_identifier +
                               " does not have a lock for the resource:" + rid.ToString() +
                               " because it is held for " +
                               static_cast<std::string>(ToBase64(it->second))));
      }
    }

    doc_store_->Set(rid, value);
  }

  void SetLaneLog2(lane_type const &count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  NewRevertibleDocumentStore *doc_store_;

  uint32_t log2_lanes_ = 0;

  uint32_t lane_assignment_ = 0;

  mutex::Mutex                                     lock_mutex_{__LINE__, __FILE__};
  std::map<byte_array::ConstByteArray, Identifier> locks_;
};

}  // namespace storage
}  // namespace fetch
