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
#include "telemetry/counter.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/utils/timer.hpp"

namespace fetch {
namespace storage {

template <typename T>
class ObjectStoreProtocol : public fetch::service::Protocol
{
public:
  using self_type = ObjectStoreProtocol<T>;

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

  ObjectStoreProtocol(TransientObjectStore<T> *obj_store, uint32_t lane)
    : obj_store_{obj_store}
    , set_count_{CreateCounter(lane, "ledger_tx_store_set_total",
                               "The total number of set operations")}
    , get_count_{CreateCounter(lane, "ledger_tx_store_get_total",
                               "The total number of get operations")}
    , set_durations_{CreateHistogram(lane, "ledger_tx_store_set_duration",
                                     "The histogram of set operation durations in seconds")}
    , get_durations_{CreateHistogram(lane, "ledger_tx_store_get_duration",
                                     "The histogram of get operation durations in seconds")}
  {
    this->Expose(GET, this, &self_type::Get);
    this->Expose(SET, this, &self_type::Set);
    this->Expose(SET_BULK, this, &self_type::SetBulk);
    this->Expose(HAS, obj_store, &TransientObjectStore<T>::Has);
    this->Expose(GET_RECENT, obj_store, &TransientObjectStore<T>::GetRecent);
  }

private:

  static telemetry::CounterPtr CreateCounter(uint32_t lane, char const *name, char const *description)
  {
    return telemetry::Registry::Instance().CreateCounter(name, description,
                                                         {{"lane", std::to_string(lane)}});
  }

  static telemetry::HistogramPtr CreateHistogram(uint32_t lane, char const *name, char const *description)
  {
    return telemetry::Registry::Instance().CreateHistogram(
        {0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1, 1, 10., 100.}, name, description,
        {{"lane", std::to_string(lane)}});
  }

  void Set(ResourceID const &rid, T const &object)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Setting object across object store protocol");

    telemetry::FunctionTimer const timer{*set_durations_};

    obj_store_->Set(rid, object, false);
    set_count_->increment();
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
    telemetry::FunctionTimer const timer{*get_durations_};

    T ret;

    if (!obj_store_->Get(rid, ret))
    {
      throw std::runtime_error("Unable to lookup element across object store protocol");
    }

    // once we have retrieved a transaction from the core it is important that we persist it to disk
    obj_store_->Confirm(rid);
    get_count_->increment();

    return ret;
  }

  TransientObjectStore<T> *obj_store_;
  telemetry::CounterPtr    set_count_;
  telemetry::CounterPtr    get_count_;
  telemetry::HistogramPtr  set_durations_;
  telemetry::HistogramPtr  get_durations_;
};

}  // namespace storage
}  // namespace fetch
