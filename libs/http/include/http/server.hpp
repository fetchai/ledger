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

#include "core/byte_array/byte_array.hpp"
#include "http/abstract_server.hpp"
#include "http/connection.hpp"
#include "http/default_root_module.hpp"
#include "http/http_connection_manager.hpp"
#include "http/method.hpp"
#include "http/mime_types.hpp"
#include "http/module.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/route.hpp"
#include "http/status.hpp"
#include "http/tagged_tree.hpp"
#include "logging/logging.hpp"
#include "network/fetch_asio.hpp"
#include "network/management/network_manager.hpp"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
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
  using HandleType = uint64_t;

  using NetworkManager    = network::NetworkManager;
  using Socket            = asio::ip::tcp::tcp::socket;
  using Acceptor          = asio::ip::tcp::tcp::acceptor;
  using ConnectionManager = HTTPConnectionManager;

  using RequestMiddleware  = std::function<void(HTTPRequest &)>;
  using ViewType           = MountedView::ViewType;
  using Authenticator      = MountedView::Authenticator;
  using ResponseMiddleware = std::function<void(HTTPResponse &, HTTPRequest const &)>;

  static constexpr char const *LOGGING_NAME = "HTTPServer";

  explicit HTTPServer(NetworkManager const &network_manager)
    : networkManager_(network_manager)
  {}

  HTTPServer(HTTPServer &&)      = delete;
  HTTPServer(HTTPServer const &) = delete;

  virtual ~HTTPServer()
  {
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

    // Since the connection manager contains a reference to this class, we must guarantee it has
    // destructed before we do.
    while (!manager_.expired())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void Start(uint16_t port)
  {
    std::weak_ptr<ConnectionManager> &manager   = manager_;
    std::weak_ptr<Socket> &           socRef    = socket_;
    std::weak_ptr<Acceptor> &         accepRef  = acceptor_;
    NetworkManager &                  threadMan = networkManager_;

    // Count instances of this shared pointer that exist to know whether the closure has executed
    std::shared_ptr<uint64_t> ref_counter = std::make_shared<uint64_t>();

    {
      HTTPServer &server_ref = *this;

      networkManager_.Post([&socRef, &accepRef, &manager, &threadMan, port, ref_counter,
                            &server_ref] {
        // Important to keep this alive during cb scope
        FETCH_UNUSED(ref_counter);

        auto soc = threadMan.CreateIO<Socket>();
        auto accep =
            threadMan.CreateIO<Acceptor>(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
        auto strong_manager = std::make_shared<ConnectionManager>(server_ref);

        FETCH_LOG_INFO(LOGGING_NAME,
                       "Starting HTTPServer on http://127.0.0.1:", accep->local_endpoint().port());

        // allow initiating class to post closes to these
        socRef   = soc;
        accepRef = accep;
        manager  = strong_manager;

        FETCH_LOG_DEBUG(LOGGING_NAME, "Starting HTTPServer Accept");
        HTTPServer::Accept(soc, accep, strong_manager);
      });
    }

    // Block until we know the closure above has either been executed or destructed as the network
    // manager wasn't ready We need to block since the closure has references to our class within
    // it.
    while (ref_counter.use_count() != 1)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  void Stop()
  {}

  void PushRequest(HandleType client, HTTPRequest req) override
  {
    // TODO(issue 35): Need to actually add better support for the options here
    if (req.method() == Method::OPTIONS)
    {

      HTTPResponse res("", fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                       Status::SUCCESS_OK);
      res.AddHeader("Access-Control-Allow-Origin", "*");
      res.AddHeader("Access-Control-Allow-Methods", "GET, PUT, POST, DELETE, OPTIONS");
      res.AddHeader("Access-Control-Allow-Headers",
                    "Content-Type, Authorization, Content-Length, X-Requested-With");

      SendToManager(client, res);
      return;
    }

    // TODO(issue 28): improve such that it works for multiple threads.
    FETCH_LOCK(eval_mutex_);
    HTTPResponse res("page not found", mime_types::GetMimeTypeFromExtension(".html"),
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
          if (!v.authenticator(req))
          {
            res = HTTPResponse("authentication required",
                               fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                               Status::SERVER_ERROR_NETWORK_AUTHENTICATION_REQUIRED);
            SendToManager(client, res);
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
    catch (std::exception const &e)
    {
      HTTPResponse response("internal error: " + std::string(e.what()),
                            fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                            Status::SERVER_ERROR_INTERNAL_SERVER_ERROR);
      SendToManager(client, response);
      return;
    }
    catch (...)
    {
      HTTPResponse response("unknown internal error",
                            fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                            Status::SERVER_ERROR_INTERNAL_SERVER_ERROR);
      SendToManager(client, response);
      return;
    }

    SendToManager(client, res);
  }

  // Accept static void to avoid having to create shared ptr to this class
  static void Accept(std::shared_ptr<Socket> const &soc, std::shared_ptr<Acceptor> const &accep,
                     std::shared_ptr<ConnectionManager> const &manager)
  {
    auto cb = [soc, accep, manager](std::error_code ec) {
      if (!ec)
      {
        assert(manager);
        auto new_connection = std::make_shared<HTTPConnection>(std::move(*soc), *manager);
        new_connection->SetHandle(manager->Join(new_connection));
        new_connection->Start();
        FETCH_LOG_DEBUG(LOGGING_NAME, "New connection formed.");
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "HTTP server terminated with ec: ", ec.message());
        return;
      }

      std::shared_ptr<Socket> const &           s = soc;
      std::shared_ptr<Acceptor> const &         a = accep;
      std::shared_ptr<ConnectionManager> const &m = manager;

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

    views_.push_back(
        {std::move(description), method, std::move(route), view, std::move(authenticator)});
  }

  void AddModule(HTTPModule const &module)
  {
    for (auto const &view : module.views())
    {
      AddView(view.description, view.method, view.route, view.parameters, view.view,
              view.authenticator);
    }
  }

  MountedViews views()
  {
    FETCH_LOCK(eval_mutex_);
    return views_unsafe();
  }

  MountedViews views_unsafe()
  {
    return views_;
  }

  void SendToManager(HandleType client, HTTPResponse const &res)
  {
    std::weak_ptr<ConnectionManager> manager = manager_;

    networkManager_.Post([manager, client, res] {
      auto manager_lock = manager.lock();

      if (manager_lock)
      {
        manager_lock->Send(client, res);
      }
    });
  }

  void AddDefaultRootModule()
  {
    AddModule(DefaultRootModule(views_));
  }

private:
  Mutex eval_mutex_;

  std::vector<RequestMiddleware>  pre_view_middleware_;
  MountedViews                    views_;
  std::vector<ResponseMiddleware> post_view_middleware_;

  NetworkManager                   networkManager_;
  std::deque<HTTPRequest>          requests_;
  std::weak_ptr<Acceptor>          acceptor_;
  std::weak_ptr<Socket>            socket_;
  std::weak_ptr<ConnectionManager> manager_;
};
}  // namespace http
}  // namespace fetch
