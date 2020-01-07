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

#include "core/assert.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/macros.hpp"
#include "core/mutex.hpp"
#include "http/abstract_connection.hpp"
#include "http/http_connection_manager.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "logging/logging.hpp"
#include "network/fetch_asio.hpp"

#include <deque>
#include <memory>

namespace fetch {
namespace http {

class HTTPConnection : public AbstractHTTPConnection,
                       public std::enable_shared_from_this<HTTPConnection>
{
public:
  using ResponseQueueType = std::deque<HTTPResponse>;
  using ConnectionType    = typename AbstractHTTPConnection::SharedType;
  using HandleType        = HTTPConnectionManager::HandleType;
  using SharedRequestType = std::shared_ptr<HTTPRequest>;
  using BufferPointerType = std::shared_ptr<asio::streambuf>;

  static constexpr char const *LOGGING_NAME = "HTTPConnection";

  HTTPConnection(asio::ip::tcp::tcp::socket socket, HTTPConnectionManager &manager)
    : socket_(std::move(socket))
    , manager_(manager)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "HTTP connection from ",
                    socket_.remote_endpoint().address().to_string());
  }

  ~HTTPConnection() override = default;

  void Start()
  {
    is_open_ = true;
    ReadHeader();
  }

  void Send(HTTPResponse const &response) override
  {
    bool write_in_progress = false;
    {
      FETCH_LOCK(write_mutex_);
      write_in_progress = !write_queue_.empty();
      write_queue_.push_back(response);
    }

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
  void ReadHeader(BufferPointerType buffer_ptr = nullptr)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Ready to ready HTTP header");

    SharedRequestType request = std::make_shared<HTTPRequest>();
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

      // only parse the header if there is data to be parsed
      if (len != 0u)
      {
        if (request->ParseHeader(*buffer_ptr, len))
        {
          if (is_open_)
          {
            ReadBody(buffer_ptr, request);
          }
        }
      }
    };

    asio::async_read_until(socket_, *buffer_ptr, "\r\n\r\n", cb);
  }

  void ReadBody(BufferPointerType const &buffer_ptr, SharedRequestType const &request)
  {
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

      if (is_open_)
      {
        ReadBody(buffer_ptr, request);
      }
    };

    asio::async_read(socket_, *buffer_ptr,
                     asio::transfer_exactly(request->content_length() - buffer_ptr->size()), cb);
  }

  void HandleError(std::error_code const &ec, SharedRequestType const & /*req*/)
  {
    std::stringstream ss;
    ss << ec << ":" << ec.message();
    FETCH_LOG_DEBUG(LOGGING_NAME, "HTTP error: ", ss.str());

    Close();
  }

  void Write()
  {
    BufferPointerType buffer_ptr =
        std::make_shared<asio::streambuf>(std::numeric_limits<std::size_t>::max());
    {
      FETCH_LOCK(write_mutex_);
      HTTPResponse res = write_queue_.front();
      write_queue_.pop_front();
      res.ToStream(*buffer_ptr);
    }

    auto self = shared_from_this();
    auto cb   = [this, self, buffer_ptr](std::error_code ec, std::size_t) {
      if (!ec)
      {
        bool write_more = false;
        {
          FETCH_LOCK(write_mutex_);
          write_more = !write_queue_.empty();
        }

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

  void CloseConnnection() override
  {
    std::error_code dummy;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, dummy);
    socket_.close(dummy);
  }

  void Close()
  {
    is_open_ = false;
    CloseConnnection();
    manager_.Leave(handle_);
  }

  void SetHandle(HandleType handle)
  {
    handle_ = handle;
  }

private:
  asio::ip::tcp::tcp::socket socket_;
  HTTPConnectionManager &    manager_;
  ResponseQueueType          write_queue_;
  Mutex                      write_mutex_;

  HandleType handle_{};
  bool       is_open_ = false;
};
}  // namespace http
}  // namespace fetch
