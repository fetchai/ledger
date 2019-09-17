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

#include "logging/logging.hpp"
#include "network/fetch_asio.hpp"
#include "oef-base/comms/Core.hpp"
#include "oef-base/comms/IMessageReader.hpp"
#include "oef-base/comms/IMessageWriter.hpp"
#include "oef-base/comms/ISocketOwner.hpp"
#include "oef-base/comms/RingBuffer.hpp"
#include "oef-base/threading/Notification.hpp"
#include "oef-base/threading/Waitable.hpp"

#include <iostream>
#include <list>

class Uri;

template <typename TXType>
class EndpointBase : public ISocketOwner, public Waitable
{
public:
  using Mutex     = std::mutex;
  using Lock      = std::lock_guard<Mutex>;
  using Socket    = asio::ip::tcp::socket;
  using ConfigMap = std::unordered_map<std::string, std::string>;

  using message_type = TXType;

  using ErrorNotification      = std::function<void(std::error_code const &ec)>;
  using EofNotification        = std::function<void()>;
  using StartNotification      = std::function<void()>;
  using ProtoErrorNotification = std::function<void(std::string const &message)>;

  using StateType  = std::atomic<int>;
  using StateTypeP = std::shared_ptr<StateType>;

  using EndpointState = enum {
    RUNNING_ENDPOINT = 1,
    CLOSED_ENDPOINT  = 2,
    EOF_ENDPOINT     = 4,
    ERRORED_ENDPOINT = 8
  };

  static constexpr std::size_t BUFFER_SIZE_LIMIT = 50;
  static constexpr char const *LOGGING_NAME      = "EndpointBase";

  /// @{
  EndpointBase(std::size_t sendBufferSize, std::size_t readBufferSize, ConfigMap configMap);
  virtual ~EndpointBase();
  EndpointBase(EndpointBase const &other) = delete;
  /// @}

  /// @{
  EndpointBase &operator=(EndpointBase const &other)  = delete;
  bool          operator==(EndpointBase const &other) = delete;
  bool          operator<(EndpointBase const &other)  = delete;
  /// @}

  virtual Socket &socket() = 0;

  virtual void go();

  std::shared_ptr<IMessageReader>         reader_;
  std::shared_ptr<IMessageWriter<TXType>> writer_;

  ErrorNotification      onError;
  EofNotification        onEof;
  StartNotification      onStart;
  ProtoErrorNotification onProtoError;

  virtual void run_sending();
  virtual void run_reading();
  virtual void close();
  virtual bool connect(const Uri &uri, Core &core);

  virtual const std::string &GetRemoteId() const
  {
    return remote_id_;
  }

  virtual Notification::NotificationBuilder send(TXType s);

  virtual bool IsTXQFull()
  {
    Lock lock(txq_mutex_);
    return txq_.size() >= BUFFER_SIZE_LIMIT;
  }

  virtual bool connected()
  {
    if (*state_ > RUNNING_ENDPOINT)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "STATE: ", state_);
    }
    return *state_ == RUNNING_ENDPOINT;
  }

  std::size_t GetIdent() const
  {
    return ident_;
  }

protected:
  virtual void async_read(std::size_t const &bytes_needed) = 0;
  virtual void async_write()                               = 0;

  virtual bool is_eof(std::error_code const &ec) const = 0;

protected:
  RingBuffer sendBuffer_;
  RingBuffer readBuffer_;

  ConfigMap configMap_;

  Mutex       mutex_;
  Mutex       txq_mutex_;
  std::size_t read_needed_ = 0;
  std::size_t ident_;

  std::string remote_id_;

  std::atomic<bool> asio_sending_;
  std::atomic<bool> asio_reading_;

  std::shared_ptr<StateType> state_;

  virtual void error(std::error_code const &ec);
  virtual void proto_error(const std::string &msg);
  virtual void eof();

  virtual void complete_sending(StateTypeP state, std::error_code const &ec,
                                const std::size_t &bytes);
  virtual void create_messages();
  virtual void complete_reading(StateTypeP state, std::error_code const &ec,
                                const std::size_t &bytes);

private:
  std::vector<Notification::Notification> waiting_;
  std::list<TXType>                       txq_;
};
