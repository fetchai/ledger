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

  using network_manager_type = network::NetworkManager;
  using socket_type          = asio::ip::tcp::tcp::socket;
  using acceptor_type        = asio::ip::tcp::tcp::acceptor;
  using manager_type         = HTTPConnectionManager;

  using request_middleware_type  = std::function<void(HTTPRequest &)>;
  using view_type                = typename HTTPModule::view_type;
  using response_middleware_type = std::function<void(HTTPResponse &, HTTPRequest const &)>;

  static constexpr char const *LOGGING_NAME = "HTTPServer";

  struct MountedView
  {
    byte_array::ConstByteArray description;
    Method                     method;
    Route                      route;
    view_type                  view;
  };

  explicit HTTPServer(network_manager_type const &network_manager)
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
    std::shared_ptr<manager_type> manager   = manager_;
    std::weak_ptr<socket_type> &  socRef    = socket_;
    std::weak_ptr<acceptor_type> &accepRef  = acceptor_;
    network_manager_type &        threadMan = networkManager_;

    networkManager_.Post([&socRef, &accepRef, manager, &threadMan, port] {
      FETCH_LOG_INFO(LOGGING_NAME, "Starting HTTPServer on http://127.0.0.1:", port);

      auto soc = threadMan.CreateIO<socket_type>();

      auto accep =
          threadMan.CreateIO<acceptor_type>(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

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
    eval_mutex_.lock();
    for (auto &m : pre_view_middleware_)
    {
      m(req);
    }

    HTTPResponse   res("page not found", mime_types::GetMimeTypeFromExtension(".html"),
                     Status::CLIENT_ERROR_NOT_FOUND);
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

    eval_mutex_.unlock();

    manager_->Send(client, res);
  }

  // Accept static void to avoid having to create shared ptr to this class
  static void Accept(std::shared_ptr<socket_type> soc, std::shared_ptr<acceptor_type> accep,
                     std::shared_ptr<manager_type> manager)
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

      std::shared_ptr<socket_type>   s = soc;
      std::shared_ptr<acceptor_type> a = accep;
      std::shared_ptr<manager_type>  m = manager;

      HTTPServer::Accept(s, a, m);
    };

    FETCH_LOG_DEBUG(LOGGING_NAME, "Starting HTTPServer async accept");
    accep->async_accept(*soc, cb);
  }

  void AddMiddleware(request_middleware_type const &middleware)
  {
    pre_view_middleware_.push_back(middleware);
  }

  void AddMiddleware(response_middleware_type const &middleware)
  {
    post_view_middleware_.push_back(middleware);
  }

  void AddView(byte_array::ConstByteArray description, Method method,
               byte_array::ByteArray const &path, std::vector<HTTPParameter> const &parameters,
               view_type const &view)
  {
    auto route = Route::FromString(path);

    for (auto const &param : parameters)
    {
      validators::Validator v = param.validator;
      v.description           = param.description;
      route.AddValidator(param.name, std::move(v));
    }

    views_.push_back({std::move(description), method, std::move(route), view});
  }

  void AddModule(HTTPModule const &module)
  {
    LOG_STACK_TRACE_POINT;
    for (auto const &view : module.views())
    {
      this->AddView(view.description, view.method, view.route, view.parameters, view.view);
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

  std::vector<request_middleware_type>  pre_view_middleware_;
  std::vector<MountedView>              views_;
  std::vector<response_middleware_type> post_view_middleware_;

  network_manager_type          networkManager_;
  std::deque<HTTPRequest>       requests_;
  std::weak_ptr<acceptor_type>  acceptor_;
  std::weak_ptr<socket_type>    socket_;
  std::shared_ptr<manager_type> manager_{std::make_shared<manager_type>(*this)};
};
}  // namespace http
}  // namespace fetch
