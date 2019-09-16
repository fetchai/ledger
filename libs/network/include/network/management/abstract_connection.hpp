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
  using SharedType           = std::shared_ptr<AbstractConnection>;
  using ConnectionHandleType = typename AbstractConnectionRegister::ConnectionHandleType;
  using WeakPointerType      = std::weak_ptr<AbstractConnection>;
  using WeakRegisterType     = std::weak_ptr<AbstractConnectionRegister>;

  static constexpr char const *LOGGING_NAME = "AbstractConnection";

  enum
  {
    TYPE_UNDEFINED = 0,
    TYPE_INCOMING  = 1,
    TYPE_OUTGOING  = 2
  };

  AbstractConnection()
    : handle_(AbstractConnection::next_handle())
  {}

  // Interface
  virtual ~AbstractConnection()
  {
    auto h = handle_.load();

    FETCH_LOG_DEBUG(LOGGING_NAME, "Connection destruction in progress for handle ");
    FETCH_LOG_VARIABLE(h);

    {
      FETCH_LOCK(callback_mutex_);
      on_message_ = nullptr;
    }

    auto ptr = connection_register_.lock();
    if (ptr)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "~AbstractConnection calling Leave");
      ptr->Leave(handle_);
    }
    FETCH_LOG_DEBUG(LOGGING_NAME, "Connection destroyed for handle ", h);
  }

  virtual void     Send(MessageType const &) = 0;
  virtual uint16_t Type() const              = 0;
  virtual void     Close()                   = 0;
  virtual bool     Closed() const            = 0;
  virtual bool     is_alive() const          = 0;

  // Common to all
  std::string Address() const
  {
    FETCH_LOCK(address_mutex_);
    return address_;
  }

  uint16_t port() const
  {
    return port_;
  }

  ConnectionHandleType handle() const noexcept
  {
    return handle_;
  }
  void SetConnectionManager(WeakRegisterType const &reg)
  {
    connection_register_ = reg;
  }

  WeakPointerType connection_pointer()
  {
    return shared_from_this();
  }

  void OnMessage(std::function<void(network::MessageType const &msg)> const &f)
  {
    FETCH_LOCK(callback_mutex_);
    on_message_ = f;
  }

  void OnConnectionSuccess(std::function<void()> const &fnc)
  {
    FETCH_LOCK(callback_mutex_);
    on_connection_success_ = fnc;
  }

  void OnConnectionFailed(std::function<void()> const &fnc)
  {
    FETCH_LOCK(callback_mutex_);
    on_connection_failed_ = fnc;
  }

  void OnLeave(std::function<void()> const &fnc)
  {
    FETCH_LOCK(callback_mutex_);
    on_leave_ = fnc;
  }

  void ClearClosures() noexcept
  {
    FETCH_LOCK(callback_mutex_);
    on_connection_failed_  = nullptr;
    on_connection_success_ = nullptr;
    on_message_            = nullptr;
  }

  void ActivateSelfManage()
  {
    self_ = shared_from_this();
  }

  void DeactivateSelfManage()
  {
    self_.reset();
  }

protected:
  void SetAddress(std::string const &addr)
  {
    FETCH_LOCK(address_mutex_);
    address_ = addr;
  }

  void SetPort(uint16_t p)
  {
    port_ = p;
  }

  void SignalLeave()
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Connection terminated for handle ", handle_.load(),
                    ", SignalLeave called.");
    std::function<void()> cb;
    {
      FETCH_LOCK(callback_mutex_);
      cb = on_leave_;
    }

    if (cb)
    {
      cb();
    }
    DeactivateSelfManage();
    FETCH_LOG_DEBUG(LOGGING_NAME, "SignalLeave is done");
  }

  void SignalMessage(network::MessageType const &msg)
  {
    std::function<void(network::MessageType const &)> cb;
    {
      FETCH_LOCK(callback_mutex_);
      cb = on_message_;
    }
    if (cb)
    {
      cb(msg);
    }
  }

  void SignalConnectionFailed()
  {
    std::function<void()> cb;
    {
      FETCH_LOCK(callback_mutex_);
      cb = on_leave_;
    }
    if (cb)
    {
      cb();
    }

    DeactivateSelfManage();
  }

  void SignalConnectionSuccess()
  {
    std::function<void()> cb;
    {
      FETCH_LOCK(callback_mutex_);
      cb = on_connection_success_;
    }

    if (cb)
    {
      cb();
    }

    DeactivateSelfManage();
  }

private:
  std::function<void(network::MessageType const &msg)> on_message_;
  std::function<void()>                                on_connection_success_;
  std::function<void()>                                on_connection_failed_;
  std::function<void()>                                on_leave_;

  std::string           address_;
  std::atomic<uint16_t> port_;

  mutable Mutex address_mutex_;

  static ConnectionHandleType next_handle()
  {
    ConnectionHandleType ret = 0;

    {
      FETCH_LOCK(global_handle_mutex_);

      while (ret == 0)
      {
        ret = global_handle_counter_++;
      }
    }

    return ret;
  }

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
