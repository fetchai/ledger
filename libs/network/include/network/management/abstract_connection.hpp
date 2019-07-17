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
      std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
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

  virtual void     Send(message_type const &) = 0;
  virtual uint16_t Type() const               = 0;
  virtual void     Close()                    = 0;
  virtual bool     Closed() const             = 0;
  virtual bool     is_alive() const           = 0;

  // Common to all
  std::string Address() const
  {
    std::lock_guard<mutex::Mutex> lock(address_mutex_);
    return address_;
  }

  uint16_t port() const
  {
    return port_;
  }

  connection_handle_type handle() const noexcept
  {
    return handle_;
  }
  void SetConnectionManager(weak_register_type const &reg)
  {
    connection_register_ = reg;
  }

  weak_ptr_type connection_pointer()
  {
    return shared_from_this();
  }

  void OnMessage(std::function<void(network::message_type const &msg)> const &f)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_message_ = f;
  }

  void OnConnectionSuccess(std::function<void()> const &fnc)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_connection_success_ = fnc;
  }

  void OnConnectionFailed(std::function<void()> const &fnc)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_connection_failed_ = fnc;
  }

  void OnLeave(std::function<void()> const &fnc)
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
    on_leave_ = fnc;
  }

  void ClearClosures() noexcept
  {
    LOG_STACK_TRACE_POINT;
    std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
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
    std::lock_guard<mutex::Mutex> lock(address_mutex_);
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
    std::function<void(void)> cb;
    {
      std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
      cb = on_leave_;
    }

    if (cb)
    {
      cb();
    }
    DeactivateSelfManage();
    FETCH_LOG_DEBUG(LOGGING_NAME, "SignalLeave is done");
  }

  void SignalMessage(network::message_type const &msg)
  {
    LOG_STACK_TRACE_POINT;
    std::function<void(network::message_type const &)> cb;
    {
      std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
      cb = on_message_;
    }
    if (cb)
    {
      cb(msg);
    }
  }

  void SignalConnectionFailed()
  {
    LOG_STACK_TRACE_POINT;
    std::function<void()> cb;
    {
      std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
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
    LOG_STACK_TRACE_POINT;
    std::function<void()> cb;
    {
      std::lock_guard<fetch::mutex::Mutex> lock(callback_mutex_);
      cb = on_connection_success_;
    }

    if (cb)
    {
      cb();
    }

    DeactivateSelfManage();
  }

private:
  std::function<void(network::message_type const &msg)> on_message_;
  std::function<void()>                                 on_connection_success_;
  std::function<void()>                                 on_connection_failed_;
  std::function<void()>                                 on_leave_;

  std::string           address_;
  std::atomic<uint16_t> port_;

  mutable mutex::Mutex address_mutex_{__LINE__, __FILE__};

  static connection_handle_type next_handle()
  {
    connection_handle_type ret = 0;

    {
      std::lock_guard<fetch::mutex::Mutex> lck(global_handle_mutex_);

      while (ret == 0)
      {
        ret = global_handle_counter_++;
      }
    }

    return ret;
  }

  weak_register_type                        connection_register_;
  std::atomic<connection_handle_type> const handle_;

  static connection_handle_type global_handle_counter_;
  static fetch::mutex::Mutex    global_handle_mutex_;
  mutable fetch::mutex::Mutex   callback_mutex_{__LINE__, __FILE__};

  shared_type self_;

  friend class AbstractConnectionRegister;
};

}  // namespace network
}  // namespace fetch
