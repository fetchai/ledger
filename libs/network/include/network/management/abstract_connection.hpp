#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
#include "core/mutex.hpp"
#include "network/management/abstract_connection_register.hpp"
#include "network/message.hpp"

#include <atomic>
#include <memory>
namespace fetch {
namespace network {

class AbstractConnection : public std::enable_shared_from_this<AbstractConnection>
{
public:
  using shared_type            = std::shared_ptr<AbstractConnection>;
  using connection_handle_type = typename AbstractConnectionRegister::connection_handle_type;
  using weak_ptr_type          = std::weak_ptr<AbstractConnection>;
  using weak_register_type     = std::weak_ptr<AbstractConnectionRegister>;

  enum
  {
    TYPE_UNDEFINED = 0,
    TYPE_INCOMING  = 1,
    TYPE_OUTGOING  = 2
  };

  AbstractConnection() { handle_ = AbstractConnection::next_handle(); }

  // Interface
  virtual ~AbstractConnection()
  {
    {
      std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
      on_message_ = nullptr;
    }

    auto ptr = connection_register_.lock();
    if (ptr)
    {
      ptr->Leave(handle_);
    }
    fetch::logger.Debug("Connection destroyed");
  }

  virtual void     Send(message_type const &) = 0;
  virtual uint16_t Type() const               = 0;
  virtual void     Close()                    = 0;
  virtual bool     Closed()                   = 0;
  virtual bool     is_alive() const           = 0;

  // Common to all
  std::string Address() const
  {
    std::lock_guard<mutex::Mutex> lock(address_mutex_);
    return address_;
  }

  uint16_t port() const { return port_; }

  connection_handle_type handle() const noexcept { return handle_; }
  void SetConnectionManager(weak_register_type const &reg) { connection_register_ = reg; }

  weak_ptr_type connection_pointer() { return shared_from_this(); }

  void OnMessage(std::function<void(network::message_type const &msg)> const &f)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_message_ = f;
  }

  void OnConnectionFailed(std::function<void()> const &fnc)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_connection_failed_ = fnc;
  }

  void OnLeave(std::function<void()> const &fnc)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_leave_ = fnc;
  }

  void ClearClosures() noexcept
  {
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_connection_failed_ = nullptr;
    on_message_           = nullptr;
  }

  void ActivateSelfManage() { self_ = shared_from_this(); }

  void DeactivateSelfManage() { self_.reset(); }

protected:
  void SetAddress(std::string const &addr)
  {
    std::lock_guard<mutex::Mutex> lock(address_mutex_);
    address_ = addr;
  }

  void SetPort(uint16_t const &p) { port_ = p; }

  void SignalLeave()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    fetch::logger.Debug("Connection terminated");

    if (on_leave_) on_leave_();
    DeactivateSelfManage();
  }

  void SignalMessage(network::message_type const &msg)
  {
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    if (on_message_) on_message_(msg);
  }

  void SignalConnectionFailed()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    if (on_connection_failed_) on_connection_failed_();

    DeactivateSelfManage();
  }

private:
  std::function<void(network::message_type const &msg)> on_message_;
  std::function<void()>                                 on_connection_failed_;
  std::function<void()>                                 on_leave_;

  std::string           address_;
  std::atomic<uint16_t> port_;

  mutable mutex::Mutex address_mutex_;

  static connection_handle_type next_handle()
  {
    std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);
    connection_handle_type               ret = global_handle_counter_;
    ++global_handle_counter_;
    return ret;
  }

  weak_register_type                  connection_register_;
  std::atomic<connection_handle_type> handle_;

  static connection_handle_type global_handle_counter_;
  static fetch::mutex::Mutex    global_handle_mutex_;

  mutable fetch::mutex::Mutex callback_mutex_;

  shared_type self_;

  friend class AbstractConnectionRegister;
};

}  // namespace network
}  // namespace fetch
