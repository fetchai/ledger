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

#include "network/management/abstract_connection.hpp"

namespace fetch {
namespace network {

AbstractConnection::ConnectionHandleType AbstractConnection::global_handle_counter_ = 1;
Mutex                                    AbstractConnection::global_handle_mutex_;

// Construction / Destruction
AbstractConnection::AbstractConnection()
  : handle_(AbstractConnection::next_handle())
{}

// Interface
AbstractConnection::~AbstractConnection()
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

// Common to allx
std::string AbstractConnection::Address() const
{
  FETCH_LOCK(address_mutex_);
  return address_;
}

uint16_t AbstractConnection::port() const
{
  return port_;
}

AbstractConnection::ConnectionHandleType AbstractConnection::handle() const noexcept
{
  return handle_;
}
void AbstractConnection::SetConnectionManager(WeakRegisterType const &reg)
{
  connection_register_ = reg;
}

AbstractConnection::WeakPointerType AbstractConnection::connection_pointer()
{
  return shared_from_this();
}

void AbstractConnection::OnMessage(std::function<void(network::MessageBuffer const &msg)> const &f)
{
  FETCH_LOCK(callback_mutex_);
  on_message_ = f;
}

void AbstractConnection::OnConnectionSuccess(std::function<void()> const &fnc)
{
  FETCH_LOCK(callback_mutex_);
  on_connection_success_ = fnc;
}

void AbstractConnection::OnConnectionFailed(std::function<void()> const &fnc)
{
  FETCH_LOCK(callback_mutex_);
  on_connection_failed_ = fnc;
}

void AbstractConnection::OnLeave(std::function<void()> const &fnc)
{
  FETCH_LOCK(callback_mutex_);
  on_leave_ = fnc;
}

void AbstractConnection::ClearClosures() noexcept
{
  FETCH_LOCK(callback_mutex_);
  on_connection_failed_  = nullptr;
  on_connection_success_ = nullptr;
  on_message_            = nullptr;
}

void AbstractConnection::ActivateSelfManage()
{
  self_ = shared_from_this();
}

void AbstractConnection::DeactivateSelfManage()
{
  self_.reset();
}

void AbstractConnection::SetAddress(std::string const &addr)
{
  FETCH_LOCK(address_mutex_);
  address_ = addr;
}

void AbstractConnection::SetPort(uint16_t p)
{
  port_ = p;
}

void AbstractConnection::SignalLeave()
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

void AbstractConnection::SignalMessage(network::MessageBuffer const &msg)
{
  std::function<void(network::MessageBuffer const &)> cb;
  {
    FETCH_LOCK(callback_mutex_);
    cb = on_message_;
  }
  if (cb)
  {
    cb(msg);
  }
}

void AbstractConnection::SignalConnectionFailed()
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

void AbstractConnection::SignalConnectionSuccess()
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

AbstractConnection::ConnectionHandleType AbstractConnection::next_handle()
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

}  // namespace network
}  // namespace fetch
