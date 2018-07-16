#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP
#include "http/connection.hpp"
#include "http/http_connection_manager.hpp"
#include "http/module.hpp"
#include "http/route.hpp"
#include "network/management/network_manager.hpp"

#include <deque>
#include <functional>
#include <map>
#include <regex>
#include <vector>

namespace fetch {
namespace http {

class HTTPServer : public AbstractHTTPServer {
 public:
  typedef uint64_t handle_type;  // TODO: Make global definition

  typedef network::NetworkManager network_manager_type;
  typedef asio::ip::tcp::tcp::socket                           socket_type;
  typedef asio::ip::tcp::tcp::acceptor                         acceptor_type;
  typedef HTTPConnectionManager                                manager_type;

  typedef std::function<void(HTTPRequest &)> request_middleware_type;
  typedef typename HTTPModule::view_type view_type;
  typedef std::function<void(HTTPResponse &, HTTPRequest const &)>
      response_middleware_type;

  struct MountedView {
    Method method;
    Route route;
    view_type view;
  };

  HTTPServer(uint16_t const &port,
             network_manager_type const &network_manager)
      : eval_mutex_(__LINE__, __FILE__),
        networkManager_(network_manager),
        request_mutex_(__LINE__, __FILE__)
  {
    LOG_STACK_TRACE_POINT;

    std::shared_ptr<manager_type> manager              = manager_;
    std::weak_ptr<socket_type>           &socRef       = socket_;
    std::weak_ptr<acceptor_type>         &accepRef     = acceptor_;
    network_manager_type                  &threadMan    = networkManager_;

    networkManager_.Post([&socRef, &accepRef, manager, &threadMan, port]
    {
      fetch::logger.Info("Starting HTTPServer");

      // TODO: (`HUT`) : fix this hack
      network_manager_type tm = threadMan;
      auto soc = threadMan.CreateIO<socket_type>();

      auto accep = threadMan.CreateIO<acceptor_type>
        (asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

      // allow initiating class to post closes to these
      socRef   = soc;
      accepRef = accep;

      fetch::logger.Info("Starting HTTPServer Accept");
      HTTPServer::Accept(soc, accep, manager);
    });

  }

  virtual ~HTTPServer() {
    LOG_STACK_TRACE_POINT;

    auto socketWeak = socket_;
    auto accepWeak  = acceptor_;

    networkManager_.Post([socketWeak, accepWeak]
      {
        auto socket = socketWeak.lock();
        auto acceptor = accepWeak.lock();
        std::error_code dummy;
        if(socket)
        {
          socket->shutdown(asio::ip::tcp::socket::shutdown_both, dummy);
          socket->close(dummy);
        }

        if(acceptor)
        {
          acceptor->close(dummy);
        }
      });

    manager_.reset();
  }

  void PushRequest(handle_type client, HTTPRequest req) override {
    LOG_STACK_TRACE_POINT;

    // TODO: improve such that it works for multiple threads.
    eval_mutex_.lock();
    for (auto &m : pre_view_middleware_) {
      m(req);
    }

    HTTPResponse res("page not found",
                     fetch::http::mime_types::GetMimeTypeFromExtension(".html"),
                     http::status_code::CLIENT_ERROR_NOT_FOUND);
    ViewParameters params;

    for (auto &v : views_) {
      if (v.route.Match(req.uri(), params)) {
        res = v.view(params, req);
        break;
      }
    }

    for (auto &m : post_view_middleware_) {
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
      //LOG_LAMBDA_STACK_TRACE_POINT; // TODO: (`HUT`) : sort this

      if (!ec) {
        std::make_shared<HTTPConnection>(std::move(*soc), *manager)
            ->Start();
      } else {
        fetch::logger.Info("HTTP server terminated with ec: ", ec.message());
        return;
      }

      std::shared_ptr<socket_type> s = soc;
      std::shared_ptr<acceptor_type> a = accep;
      std::shared_ptr<manager_type> m = manager;

      HTTPServer::Accept(s, a, m);
    };

      fetch::logger.Info("Starting HTTPServer async accept");
    accep->async_accept(*soc, cb);
  }

  void AddMiddleware(request_middleware_type const &middleware) {
    pre_view_middleware_.push_back(middleware);
  }

  void AddMiddleware(response_middleware_type const &middleware) {
    post_view_middleware_.push_back(middleware);
  }

  void AddView(Method method, byte_array::ByteArray const &path,
               view_type const &view) {
    views_.push_back({method, Route::FromString(path), view});
  }

  void AddModule(HTTPModule const &module) {
    LOG_STACK_TRACE_POINT;
    for (auto const &view : module.views()) {
      this->AddView(view.method, view.route, view.view);
    }
  }

 private:
  fetch::mutex::Mutex eval_mutex_;

  std::vector<request_middleware_type> pre_view_middleware_;
  std::vector<MountedView> views_;
  std::vector<response_middleware_type> post_view_middleware_;

  network_manager_type                    networkManager_;
  std::deque<HTTPRequest>                requests_;
  fetch::mutex::Mutex                    request_mutex_;
  std::weak_ptr<acceptor_type>           acceptor_;
  std::weak_ptr<socket_type>             socket_;
  std::shared_ptr<manager_type>          manager_{std::make_shared<manager_type>(*this)};
};
}
}

#endif
