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

#include "network/service/protocol.hpp"
#include "storage/object_store_protocol.hpp"
#include "storage/transient_object_store.hpp"

#include <functional>

namespace fetch {
namespace storage {

template <typename T>
class ObjectStoreProtocol : public fetch::service::Protocol
{
public:
  using event_set_object_type = std::function<void(T const &)>;
  using self_type             = ObjectStoreProtocol<T>;

  static constexpr char const *LOGGING_NAME = "ObjectStoreProto";

  struct Element
  {
    ResourceID key;
    T          value;

    template <typename S>
    friend void Serialize(S &s, Element const &e)
    {
      s << e.key << e.value;
    }

    template <typename S>
    friend void Deserialize(S &s, Element &e)
    {
      s >> e.key >> e.value;
    }
  };

  using ElementList = std::vector<Element>;

  enum
  {
    GET = 0,
    SET,
    SET_BULK,
    HAS,
    GET_RECENT
  };

  ObjectStoreProtocol(TransientObjectStore<T> *obj_store)
    : fetch::service::Protocol()
  {
    obj_store_ = obj_store;
    this->Expose(GET, this, &self_type::Get);
    this->Expose(SET, this, &self_type::Set);
    this->Expose(SET_BULK, this, &self_type::SetBulk);
    this->Expose(HAS, obj_store, &TransientObjectStore<T>::Has);
    this->Expose(GET_RECENT, obj_store, &TransientObjectStore<T>::GetRecent);
  }

  void OnSetObject(event_set_object_type const &f)
  {
    on_set_ = f;
  }

private:
  void Set(ResourceID const &rid, T const &object)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Setting object across object store protocol");

    if (on_set_)
    {
      on_set_(object);
    }

    obj_store_->Set(rid, object, false);
  }

  void SetBulk(ElementList const &elements)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Setting multiple objects across object store protocol");

    // loop through and set all the values
    for (Element const &element : elements)
    {
      Set(element.key, element.value);
    }
  }

  T Get(ResourceID const &rid)
  {
    T ret;

    if (!obj_store_->Get(rid, ret))
    {
      throw std::runtime_error("Unable to lookup element across object store protocol");
    }

    // once we have retrieved a transaction from the core it is important that we persist it to disk
    obj_store_->Confirm(rid);

    return ret;
  }

  TransientObjectStore<T> *obj_store_;
  event_set_object_type    on_set_;
};

}  // namespace storage
}  // namespace fetch
