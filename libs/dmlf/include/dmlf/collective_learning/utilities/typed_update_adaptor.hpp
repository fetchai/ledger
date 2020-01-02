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

#include <memory>

#include "core/mutex.hpp"

#include "dmlf/colearn/abstract_message_controller.hpp"
#include "dmlf/collective_learning/utilities/type_map.hpp"

#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace dmlf {
namespace collective_learning {
namespace utilities {

class TypedUpdateAdaptor
{
public:
  using MessageControllerPtr = std::shared_ptr<colearn::AbstractMessageController>;

  explicit TypedUpdateAdaptor(MessageControllerPtr msg_ctrl)
    : msg_ctrl_{std::move(msg_ctrl)}
  {}

  template <typename T>
  void RegisterUpdateType(std::string const &update_type)
  {
    FETCH_LOCK(update_types_m_);
    update_types_.template put<T>(update_type);
  }

  template <typename T>
  void PushUpdate(std::shared_ptr<T> const &update)
  {
    std::string upd_type{};
    {
      FETCH_LOCK(update_types_m_);
      upd_type = update_types_.template find<T>();
    }

    auto data = Serialize(update);

    msg_ctrl_->PushUpdate(data, "algo0", upd_type);
  }

  template <typename T>
  std::size_t GetUpdateCount() const
  {
    std::string upd_type{};
    {
      FETCH_LOCK(update_types_m_);
      upd_type = update_types_.template find<T>();
    }

    return msg_ctrl_->GetUpdateCount("alog0", upd_type);
  }

  template <typename T>
  std::shared_ptr<T> GetUpdate()
  {
    std::string upd_type{};
    {
      FETCH_LOCK(update_types_m_);
      upd_type = update_types_.template find<T>();
    }

    Bytes bytes = msg_ctrl_->GetUpdate("algo0", upd_type)->data();
    return Deserialize<T>(bytes);
  }

  virtual ~TypedUpdateAdaptor()                       = default;
  TypedUpdateAdaptor(TypedUpdateAdaptor const &other) = delete;
  TypedUpdateAdaptor(TypedUpdateAdaptor &&other)      = delete;
  TypedUpdateAdaptor &operator=(TypedUpdateAdaptor const &other) = delete;
  TypedUpdateAdaptor &operator=(TypedUpdateAdaptor &&other) = delete;

private:
  using Bytes = colearn::AbstractMessageController::Bytes;
  using Mutex = fetch::Mutex;
  using Lock  = std::unique_lock<Mutex>;

  MessageControllerPtr msg_ctrl_;
  TypeMap<>            update_types_;
  mutable Mutex        update_types_m_;

  template <typename T>
  Bytes Serialize(std::shared_ptr<T> const &update)
  {
    fetch::serializers::MsgPackSerializer serializer;
    serializer << *update;
    return serializer.data();
  }

  template <typename T>
  std::shared_ptr<T> Deserialize(Bytes const &bytes)
  {
    auto                                  update = std::make_shared<T>();
    fetch::serializers::MsgPackSerializer deserializer{bytes};
    deserializer >> *update;
    return update;
  }
};

}  // namespace utilities
}  // namespace collective_learning
}  // namespace dmlf
}  // namespace fetch
