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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "http/abstract_connection.hpp"
#include "http/http_connection_manager.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "network/fetch_asio.hpp"

#include <deque>
#include <memory>

namespace fetch {
namespace http {

class HTTPConnection : public AbstractHTTPConnection,
                       public std::enable_shared_from_this<HTTPConnection>
{
public:
  using response_queue_type = std::deque<HTTPResponse>;
  using connection_type     = typename AbstractHTTPConnection::shared_type;
  using handle_type         = HTTPConnectionManager::handle_type;
  using shared_request_type = std::shared_ptr<HTTPRequest>;
  using buffer_ptr_type     = std::shared_ptr<asio::streambuf>;

  static constexpr char const *LOGGING_NAME = "HTTPConnection";

  HTTPConnection(asio::ip::tcp::tcp::socket socket, HTTPConnectionManager &manager)
    : socket_(std::move(socket))
    , manager_(manager)
    , write_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_DEBUG(LOGGING_NAME, "HTTP connection from ",
                    socket_.remote_endpoint().address().to_string());
  }

  ~HTTPConnection()
  {
    LOG_STACK_TRACE_POINT;

    manager_.Leave(handle_);
  }

  void Start()
  {
    LOG_STACK_TRACE_POINT;

    is_open_ = true;
    handle_  = manager_.Join(shared_from_this());
    if (is_open_)
    {
      ReadHeader();
    }
  }

  void Send(HTTPResponse const &response) override
  {
    LOG_STACK_TRACE_POINT;

    write_mutex_.lock();
    bool write_in_progress = !write_queue_.empty();
    write_queue_.push_back(response);
    write_mutex_.unlock();

    if (!write_in_progress)
    {
      Write();
    }
  }

  std::string Address() override
  {
    return socket_.remote_endpoint().address().to_string();
  }

  asio::ip::tcp::tcp::socket &socket()
  {
    return socket_;
  }

public:
  void ReadHeader(buffer_ptr_type buffer_ptr = nullptr)
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_DEBUG(LOGGING_NAME, "Ready to ready HTTP header");

    shared_request_type request = std::make_shared<HTTPRequest>();
    if (!buffer_ptr)
    {
      buffer_ptr = std::make_shared<asio::streambuf>(std::numeric_limits<std::size_t>::max());
    }

    auto self = shared_from_this();

    auto cb = [this, buffer_ptr, request, self](std::error_code const &ec, std::size_t len) {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Read HTTP header");
      FETCH_LOG_DEBUG(LOGGING_NAME, "Read HTTP header of " + std::to_string(len) + " bytes");

      if (ec)
      {
        this->HandleError(ec, request);
        return;
      }
      else
      {
        // only parse the header if there is data to be parsed
        if (len)
        {
          if (request->ParseHeader(*buffer_ptr, len))
          {
            if (is_open_)
            {
              ReadBody(buffer_ptr, request);
            }
          }
        }
      }
    };

    asio::async_read_until(socket_, *buffer_ptr, "\r\n\r\n", cb);
  }

  void ReadBody(buffer_ptr_type buffer_ptr, shared_request_type request)
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_DEBUG(LOGGING_NAME, "Read HTTP body");
    // Check if we got all the body
    if (request->content_length() <= buffer_ptr->size())
    {
      request->ParseBody(*buffer_ptr);

      // at this point if the read has been successful populate the remote address information
      // inside the request
      auto const &remote_endpoint = socket_.remote_endpoint();
      request->SetOriginatingAddress(remote_endpoint.address().to_string(), remote_endpoint.port());

      // push the request to the main server
      manager_.PushRequest(handle_, *request);

      if (is_open_)
      {
        ReadHeader(buffer_ptr);
      }
      return;
    }

    // Reading remaining bits if not all was read.
    auto self = shared_from_this();
    auto cb   = [this, buffer_ptr, request, self](std::error_code const &ec, std::size_t len) {
      FETCH_UNUSED(len);

      FETCH_LOG_DEBUG(LOGGING_NAME, "Read HTTP body cb");
      if (ec)
      {
        this->HandleError(ec, request);
        return;
      }
      else
      {
        if (is_open_)
        {
          ReadBody(buffer_ptr, request);
        }
      }
    };

    asio::async_read(socket_, *buffer_ptr,
                     asio::transfer_exactly(request->content_length() - buffer_ptr->size()), cb);
  }

  void HandleError(std::error_code const &ec, shared_request_type /*req*/)
  {
    LOG_STACK_TRACE_POINT;

    std::stringstream ss;
    ss << ec << ":" << ec.message();
    FETCH_LOG_DEBUG(LOGGING_NAME, "HTTP error: ", ss.str());

    Close();
  }

  void Write()
  {
    LOG_STACK_TRACE_POINT;

    buffer_ptr_type buffer_ptr =
        std::make_shared<asio::streambuf>(std::numeric_limits<std::size_t>::max());
    write_mutex_.lock();
    HTTPResponse res = write_queue_.front();
    write_queue_.pop_front();
    write_mutex_.unlock();

    res.ToStream(*buffer_ptr);
    auto self = shared_from_this();
    auto cb   = [this, self, buffer_ptr](std::error_code ec, std::size_t) {
      if (!ec)
      {
        write_mutex_.lock();
        bool write_more = !write_queue_.empty();
        write_mutex_.unlock();
        if (is_open_ && write_more)
        {
          Write();
        }
      }
      else
      {
        manager_.Leave(handle_);
      }
    };

    asio::async_write(socket_, *buffer_ptr, cb);
  }

  void Close()
  {
    LOG_STACK_TRACE_POINT;

    is_open_ = false;
    manager_.Leave(handle_);
  }

private:
  asio::ip::tcp::tcp::socket socket_;
  HTTPConnectionManager &    manager_;
  response_queue_type        write_queue_;
  fetch::mutex::Mutex        write_mutex_;

  handle_type handle_;
  bool        is_open_ = false;
};
}  // namespace http
}  // namespace fetch
