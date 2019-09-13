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

#include "core/logging.hpp"
#include "core/mutex.hpp"
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
  using shared_type            = std::shared_ptr<AbstractConnection>;
  using connection_handle_type = typename AbstractConnectionRegister::connection_handle_type;
  using weak_ptr_type          = std::weak_ptr<AbstractConnection>;
  using weak_register_type     = std::weak_ptr<AbstractConnectionRegister>;

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

  virtual void     Send(message_type const &) = 0;
  virtual uint16_t Type() const               = 0;
  virtual void     Close()                    = 0;
  virtual bool     Closed() const             = 0;
  virtual bool     is_alive() const           = 0;

  // Common to allx
  std::string            Address() const;
  uint16_t               port() const;
  connection_handle_type handle() const noexcept;
  void                   SetConnectionManager(weak_register_type const &reg);

  weak_ptr_type connection_pointer();

  void OnMessage(std::function<void(network::message_type const &msg)> const &f);

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
  void SignalMessage(network::message_type const &msg);
  void SignalConnectionFailed();
  void SignalConnectionSuccess();

private:
  std::function<void(network::message_type const &msg)> on_message_;
  std::function<void()>                                 on_connection_success_;
  std::function<void()>                                 on_connection_failed_;
  std::function<void()>                                 on_leave_;

  std::string           address_{};
  std::atomic<uint16_t> port_{0};

  mutable Mutex address_mutex_;

  static connection_handle_type next_handle();

  weak_register_type                        connection_register_;
  std::atomic<connection_handle_type> const handle_;

  static connection_handle_type global_handle_counter_;
  static Mutex                  global_handle_mutex_;
  mutable Mutex                 callback_mutex_;

  shared_type self_;

  friend class AbstractConnectionRegister;
};

}  // namespace network
}  // namespace fetch
