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
#include "http/http_connection_manager.hpp"
#include "http/method.hpp"
#include "http/mime_types.hpp"
#include "http/module.hpp"
#include "http/request.hpp"
#include "http/response.hpp"
#include "http/route.hpp"
#include "http/status.hpp"
#include "logging/logging.hpp"
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
  using HandleType = uint64_t;

  using NetworkManager    = network::NetworkManager;
  using Socket            = asio::ip::tcp::tcp::socket;
  using Acceptor          = asio::ip::tcp::tcp::acceptor;
  using ConnectionManager = HTTPConnectionManager;

  using RequestMiddleware  = std::function<void(HTTPRequest &)>;
  using ViewType           = typename HTTPModule::ViewType;
  using Authenticator      = typename HTTPModule::Authenticator;
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

  explicit HTTPServer(NetworkManager const &network_manager);

  virtual ~HTTPServer();

  void Start(uint16_t port);

  void Stop();

  void PushRequest(HandleType client, HTTPRequest req) override;

  // Accept static void to avoid having to create shared ptr to this class
  static void Accept(std::shared_ptr<Socket> const &soc, std::shared_ptr<Acceptor> const &accep,
                     std::shared_ptr<ConnectionManager> const &manager);

  void AddMiddleware(RequestMiddleware const &middleware);

  void AddMiddleware(ResponseMiddleware const &middleware);

  void AddView(byte_array::ConstByteArray description, Method method,
               byte_array::ByteArray const &path, std::vector<HTTPParameter> const &parameters,
               ViewType const &view, Authenticator authenticator);

  void AddModule(HTTPModule const &module);

  std::vector<MountedView> views();

  std::vector<MountedView> views_unsafe();

private:
  void SendToManager(HandleType client, HTTPResponse const &res);

  Mutex eval_mutex_;

  std::vector<RequestMiddleware>  pre_view_middleware_;
  std::vector<MountedView>        views_;
  std::vector<ResponseMiddleware> post_view_middleware_;

  NetworkManager                   networkManager_;
  std::deque<HTTPRequest>          requests_;
  std::weak_ptr<Acceptor>          acceptor_;
  std::weak_ptr<Socket>            socket_;
  std::weak_ptr<ConnectionManager> manager_;
};
}  // namespace http
}  // namespace fetch
