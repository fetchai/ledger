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

#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"
#include "http/abstract_server.hpp"
#include "http/connection.hpp"
#include "http/http_connection_manager.hpp"
#include "http/method.hpp"
#include "http/mime_types.hpp"
#include "http/module.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/route.hpp"
#include "http/status.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/network_manager.hpp"

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <system_error>
#include <utility>
#include <vector>

namespace fetch {
namespace http {

class HTTPServer : public AbstractHTTPServer
{
public:
  using handle_type = uint64_t;

  using NetworkManager = network::NetworkManager;
  using Socket          = asio::ip::tcp::tcp::socket;
  using Acceptor        = asio::ip::tcp::tcp::acceptor;
  using ConnectionManager         = HTTPConnectionManager;

  using RequestMiddleware  = std::function<void(HTTPRequest &)>;
  using ViewType                = typename HTTPModule::ViewType;
  using Authenticator                = typename HTTPModule::Authenticator;  
  using ResponseMiddleware = std::function<void(HTTPResponse &, HTTPRequest const &)>;

  static constexpr char const *LOGGING_NAME = "HTTPServer";

  struct MountedView
  {
    byte_array::ConstByteArray description;
    Method                     method;
    Route                      route;
    ViewType                   view;
    Authenticator              authenticator;
  };

  explicit HTTPServer(NetworkManager const &network_manager)
    : networkManager_(network_manager)
  {
    LOG_STACK_TRACE_POINT;
  }

  virtual ~HTTPServer()
  {
    LOG_STACK_TRACE_POINT;

    auto socketWeak = socket_;
    auto accepWeak  = acceptor_;

    networkManager_.Post([socketWeak, accepWeak] {
      auto            socket   = socketWeak.lock();
      auto            acceptor = accepWeak.lock();
      std::error_code dummy;
      if (socket)
      {
        socket->shutdown(asio::ip::tcp::socket::shutdown_both, dummy);
        socket->close(dummy);
      }

      if (acceptor)
      {
        acceptor->close(dummy);
      }
    });

    // TODO (issue 1220): This appears to cause a double free due to a race
    /* manager_.reset(); */
  }

  void Start(uint16_t port)
  {
    std::shared_ptr<ConnectionManager> manager   = manager_;
    std::weak_ptr<Socket> &  socRef    = socket_;
    std::weak_ptr<Acceptor> &accepRef  = acceptor_;
    NetworkManager &        threadMan = networkManager_;

    networkManager_.Post([&socRef, &accepRef, manager, &threadMan, port] {
      FETCH_LOG_INFO(LOGGING_NAME, "Starting HTTPServer on http://127.0.0.1:", port);

      auto soc = threadMan.CreateIO<Socket>();

      auto accep =
          threadMan.CreateIO<Acceptor>(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

      // allow initiating class to post closes to these
      socRef   = soc;
      accepRef = accep;

      FETCH_LOG_DEBUG(LOGGING_NAME, "Starting HTTPServer Accept");
      HTTPServer::Accept(soc, accep, manager);
    });
  }

  void Stop()
  {}

  void PushRequest(handle_type client, HTTPRequest req) override
  {
    LOG_STACK_TRACE_POINT;

    // TODO(issue 35): Need to actually add better support for the options here
    if (req.method() == Method::OPTIONS)
    {

      HTTPResponse res("", fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                       Status::SUCCESS_OK);
      res.AddHeader("Access-Control-Allow-Origin", "*");
      res.AddHeader("Access-Control-Allow-Methods", "GET, PUT, POST, DELETE, OPTIONS");
      res.AddHeader("Access-Control-Allow-Headers",
                    "Content-Type, Authorization, Content-Length, X-Requested-With");

      manager_->Send(client, res);
      return;
    }

    // TODO(issue 28): improve such that it works for multiple threads.
    std::lock_guard<std::mutex> lock(eval_mutex_);
    HTTPResponse   res("page not found", mime_types::GetMimeTypeFromExtension(".html"),
                     Status::CLIENT_ERROR_NOT_FOUND);

    // Ensure that the HTTP server remains operational
    // even if exceptions are thrown
    try
    {
      // applying pre-process middleware
      for (auto &m : pre_view_middleware_)
      {
        m(req);
      }

      // finding the view that matches the URL
      ViewParameters params;
      for (auto &v : views_)
      {
        // skip all views that don't match the required method
        if (v.method != req.method())
        {
          continue;
        }

        if (v.route.Match(req.uri(), params))
        {
          // checking that the correct level of authentication is present
          if(!v.authenticator(req))
          {
            res = HTTPResponse("authentication required", fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                   Status::SERVER_ERROR_NETWORK_AUTHENTICATION_REQUIRED);
            manager_->Send(client, res);
            return;
          }

          // generating result
          res = v.view(params, req);
          break;
        }
      }

      // signal that the request has been processed
      req.SetProcessed();

      for (auto &m : post_view_middleware_)
      {
        m(res, req);
      }

    }
    catch(std::exception const &e)
    {
      HTTPResponse res("internal error: " + std::string(e.what()), fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                       Status::SERVER_ERROR_INTERNAL_SERVER_ERROR);
      manager_->Send(client, res);
      return;      
    }
    catch(...)
    {
      HTTPResponse res("unknown internal error", fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                       Status::SERVER_ERROR_INTERNAL_SERVER_ERROR);
      manager_->Send(client, res);
      return;      
    }

    manager_->Send(client, res);
  }

  // Accept static void to avoid having to create shared ptr to this class
  static void Accept(std::shared_ptr<Socket> soc, std::shared_ptr<Acceptor> accep,
                     std::shared_ptr<ConnectionManager> manager)
  {
    LOG_STACK_TRACE_POINT;

    auto cb = [soc, accep, manager](std::error_code ec) {
      // LOG_LAMBDA_STACK_TRACE_POINT; // TODO(issue 28) : sort this

      if (!ec)
      {
        std::make_shared<HTTPConnection>(std::move(*soc), *manager)->Start();
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "HTTP server terminated with ec: ", ec.message());
        return;
      }

      std::shared_ptr<Socket>   s = soc;
      std::shared_ptr<Acceptor> a = accep;
      std::shared_ptr<ConnectionManager>  m = manager;

      HTTPServer::Accept(s, a, m);
    };

    FETCH_LOG_DEBUG(LOGGING_NAME, "Starting HTTPServer async accept");
    accep->async_accept(*soc, cb);
  }

  void AddMiddleware(RequestMiddleware const &middleware)
  {
    pre_view_middleware_.push_back(middleware);
  }

  void AddMiddleware(ResponseMiddleware const &middleware)
  {
    post_view_middleware_.push_back(middleware);
  }

  void AddView(byte_array::ConstByteArray description, Method method,
               byte_array::ByteArray const &path, std::vector<HTTPParameter> const &parameters,
               ViewType const &view, Authenticator authenticator)
  {
    auto route = Route::FromString(path);

    for (auto const &param : parameters)
    {
      validators::Validator v = param.validator;
      v.description           = param.description;
      route.AddValidator(param.name, std::move(v));
    }

    views_.push_back({std::move(description), method, std::move(route), view, authenticator});
  }

  void AddModule(HTTPModule const &module)
  {
    LOG_STACK_TRACE_POINT;
    for (auto const &view : module.views())
    {
      this->AddView(view.description, view.method, view.route, view.parameters, view.view, view.authenticator);
    }
  }

  std::vector<MountedView> views()
  {
    std::lock_guard<std::mutex> lock(eval_mutex_);
    return views_;
  }

  std::vector<MountedView> views_unsafe()
  {
    return views_;
  }

private:
  std::mutex eval_mutex_;

  std::vector<RequestMiddleware>  pre_view_middleware_;
  std::vector<MountedView>              views_;
  std::vector<ResponseMiddleware> post_view_middleware_;

  NetworkManager          networkManager_;
  std::deque<HTTPRequest>       requests_;
  std::weak_ptr<Acceptor>  acceptor_;
  std::weak_ptr<Socket>    socket_;
  std::shared_ptr<ConnectionManager> manager_{std::make_shared<ConnectionManager>(*this)};
};
}  // namespace http
}  // namespace fetch
