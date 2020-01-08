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

#include "core/mutex.hpp"
#include "logging/logging.hpp"
#include "network/management/abstract_connection_register.hpp"
#include "network/message.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace fetch {
namespace network {

class AbstractConnection : public std::enable_shared_from_this<AbstractConnection>
{
public:
  using SharedType           = std::shared_ptr<AbstractConnection>;
  using ConnectionHandleType = typename AbstractConnectionRegister::ConnectionHandleType;
  using WeakPointerType      = std::weak_ptr<AbstractConnection>;
  using WeakRegisterType     = std::weak_ptr<AbstractConnectionRegister>;
  using Callback             = MessageType::Callback;

  static constexpr char const *LOGGING_NAME = "AbstractConnection";

  enum
  {
    TYPE_UNDEFINED = 0,
    TYPE_INCOMING  = 1,
    TYPE_OUTGOING  = 2
  };

  // Construction / Destruction
  AbstractConnection();

  // Interface
  virtual ~AbstractConnection();

  virtual void     Send(MessageBuffer const &, Callback const &success = nullptr,
                        Callback const &fail = nullptr) = 0;
  virtual uint16_t Type() const                         = 0;
  virtual void     Close()                              = 0;
  virtual bool     Closed() const                       = 0;
  virtual bool     is_alive() const                     = 0;

  // Common to allx
  std::string          Address() const;
  uint16_t             port() const;
  ConnectionHandleType handle() const noexcept;
  void                 SetConnectionManager(WeakRegisterType const &reg);

  WeakPointerType connection_pointer();

  void OnMessage(std::function<void(network::MessageBuffer const &msg)> const &f);

  void OnConnectionSuccess(std::function<void()> const &fnc);

  void OnConnectionFailed(std::function<void()> const &fnc);
  void OnLeave(std::function<void()> const &fnc);

  void ClearClosures() noexcept;
  void ActivateSelfManage();
  void DeactivateSelfManage();

protected:
  void SetAddress(std::string const &addr);
  void SetPort(uint16_t p);

  void SignalLeave();
  void SignalMessage(network::MessageBuffer const &msg);
  void SignalConnectionFailed();
  void SignalConnectionSuccess();

private:
  std::function<void(network::MessageBuffer const &msg)> on_message_;
  std::function<void()>                                  on_connection_success_;
  std::function<void()>                                  on_connection_failed_;
  std::function<void()>                                  on_leave_;

  std::string           address_{};
  std::atomic<uint16_t> port_{0};

  mutable Mutex address_mutex_;

  static ConnectionHandleType next_handle();

  WeakRegisterType                        connection_register_;
  std::atomic<ConnectionHandleType> const handle_;

  static ConnectionHandleType global_handle_counter_;
  static Mutex                global_handle_mutex_;
  mutable Mutex               callback_mutex_;

  SharedType self_;

  friend class AbstractConnectionRegister;
};

}  // namespace network
}  // namespace fetch
