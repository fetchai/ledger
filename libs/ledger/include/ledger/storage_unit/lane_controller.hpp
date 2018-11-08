#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/service_client.hpp"
#include "network/uri.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace fetch {
namespace ledger {

class LaneController
{
public:
  using connectivity_details_type  = LaneConnectivityDetails;
  using Uri                        = fetch::network::Uri;
  using client_type                = fetch::network::TCPClient;
  using service_client_type        = fetch::service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using weak_service_client_type   = std::shared_ptr<service_client_type>;
  using client_register_type       = fetch::network::ConnectionRegister<connectivity_details_type>;
  using network_manager_type       = fetch::network::NetworkManager;
  using mutex_type                 = fetch::mutex::Mutex;
  using connection_handle_type     = client_register_type::connection_handle_type;
  using protocol_handler_type      = service::protocol_handler_type;
  using Clock                      = std::chrono::steady_clock;
  using thread_pool_type           = network::ThreadPool;
  using UriSet                     = std::unordered_set<Uri>;

  static constexpr char const *LOGGING_NAME = "LaneController";

  LaneController(protocol_handler_type lane_identity_protocol, std::weak_ptr<LaneIdentity> identity,
                 client_register_type reg, network_manager_type const &nm)
    : lane_identity_protocol_(lane_identity_protocol)
    , lane_identity_(std::move(identity))
    , register_(std::move(reg))
    , manager_(nm)
  {
    thread_pool_ = network::MakeThreadPool(3);
    thread_pool_->SetIdleInterval(1000);
    thread_pool_->Start();
    thread_pool_->Post([this]() { thread_pool_->PostIdle([this]() { this->WorkCycle(); }); }, 1000);
  }

  /// External controls
  /// @{
  void RPCConnectToURIs(const std::vector<Uri> &uris)
  {
    for (auto const &uri : uris)
    {
      FETCH_LOG_VARIABLE(uri);
      FETCH_LOG_INFO(LOGGING_NAME, "WILL ATTEMPT TO CONNECT TO: ", uri.uri());
    }
  }

  void Shutdown()
  {
    TODO_FAIL("Needs to be implemented");
  }

  void StartSync()
  {
    TODO_FAIL("Needs to be implemented");
  }

  void StopSync()
  {
    TODO_FAIL("Needs to be implemented");
  }

  int IncomingPeers()
  {
    FETCH_LOCK(services_mutex_);

    int incoming = 0;
    for (auto &peer : services_)
    {
      auto details = register_.GetDetails(peer.first);
      {
        if (details->is_peer && (!details->is_outgoing))
        {
          ++incoming;
        }
      }
    }
    return incoming;
  }

  int OutgoingPeers()
  {
    FETCH_LOCK(services_mutex_);

    int outgoing = 0;
    for (auto &peer : services_)
    {
      auto details = register_.GetDetails(peer.first);
      {
        if (details->is_peer && details->is_outgoing)
        {
          ++outgoing;
        }
      }
    }
    return outgoing;
  }

  /// @}

  /// Internal controls
  /// @{
  shared_service_client_type GetClient(connection_handle_type const &n)
  {
    FETCH_LOCK(services_mutex_);
    return services_[n];
  }

  using IdentifiedPeer = std::pair<shared_service_client_type, crypto::Identity>;
  using LanedPeer      = std::pair<shared_service_client_type, LaneIdentity::lane_type>;

  class IdentifyingConnection : public network::ResolvableTo<IdentifiedPeer>
  {
  public:
    IdentifyingConnection(shared_service_client_type conn,
                          protocol_handler_type      lane_identity_protocol,
                          crypto::Identity           my_identity)
    {
      conn_                   = conn;
      lane_identity_protocol_ = lane_identity_protocol;
      my_identity_            = my_identity;
    }
    IdentifyingConnection(const IdentifyingConnection &other)
      : ResolvableTo<IdentifiedPeer>(other)
    {
      if (this != &other)
      {
        this->prom_                   = other.prom_;
        this->conn_                   = other.conn_;
        this->timeout_                = other.timeout_;
        this->my_identity_            = other.my_identity_;
        this->lane_identity_protocol_ = other.lane_identity_protocol_;
      }
    }

    shared_service_client_type GetConn()
    {
      return conn_;
    }

    virtual State GetState() override
    {
      return GetState(Clock::now());
    }

    virtual State GetState(const Timepoint &now) override
    {
      if (prom_.empty())
      {
        auto p = conn_->Call(lane_identity_protocol_, LaneIdentityProtocol::HELLO, my_identity_);
        prom_.Adopt(p);
        timeout_.SetMilliseconds(now, 100);
        return State::WAITING;
      }
      switch (prom_.GetState())
      {
      case State::WAITING:
        if (timeout_.IsDue(now))
        {
          return State::TIMEDOUT;
        }
        else
        {
          return State::WAITING;
        }

      case State::TIMEDOUT:
      case State::FAILED:
        return State::FAILED;

      case State::SUCCESS:
        return State::SUCCESS;
      }

      return State::FAILED;
    }
    virtual PromiseCounter id() const override
    {
      return prom_.id();
    }
    virtual IdentifiedPeer Get() const override
    {
      return IdentifiedPeer(conn_, prom_.Get());
    }

  private:
    network::PromiseOf<crypto::Identity> prom_;
    shared_service_client_type           conn_;
    network::FutureTimepoint             timeout_;
    protocol_handler_type                lane_identity_protocol_;
    crypto::Identity                     my_identity_;
  };

  class LaningConnection : public network::ResolvableTo<LanedPeer>
  {
  public:
    LaningConnection(shared_service_client_type conn, protocol_handler_type lane_identity_protocol)
    {
      conn_                   = conn;
      lane_identity_protocol_ = lane_identity_protocol;
    }
    LaningConnection(const LaningConnection &other)
      : ResolvableTo<LanedPeer>(other)
    {
      if (this != &other)
      {
        this->prom_                   = other.prom_;
        this->conn_                   = other.conn_;
        this->lane_identity_protocol_ = other.lane_identity_protocol_;
        this->timeout_                = other.timeout_;
      }
    }

    shared_service_client_type GetConn()
    {
      return conn_;
    }

    virtual State GetState() override
    {
      return GetState(Clock::now());
    }

    virtual State GetState(const Timepoint &now) override
    {
      if (prom_.empty())
      {
        auto p = conn_->Call(lane_identity_protocol_, LaneIdentityProtocol::GET_LANE_NUMBER);
        prom_.Adopt(p);
        timeout_.SetMilliseconds(now, 100);
        return State::WAITING;
      }

      switch (prom_.GetState())
      {
      case State::WAITING:
        return State::WAITING;

      case State::TIMEDOUT:
      case State::SUCCESS:
        return State::SUCCESS;

      case State::FAILED:
      default:
        return State::FAILED;
      }
    }

    virtual PromiseCounter id() const override
    {
      return prom_.id();
    }
    virtual LanedPeer Get() const override
    {
      return LanedPeer(conn_, prom_.Get());
    }

  private:
    network::PromiseOf<LaneIdentity::lane_type> prom_;
    shared_service_client_type                  conn_;
    protocol_handler_type                       lane_identity_protocol_;
    network::FutureTimepoint                    timeout_;
  };

  using IdentifyingPeers = network::RequestingQueueOf<Uri, IdentifiedPeer, IdentifyingConnection>;
  using LaningPeers      = network::RequestingQueueOf<Uri, LanedPeer, LaningConnection>;

  IdentifyingPeers currently_identifying;
  LaningPeers      currently_laning;

  void WorkCycle()
  {
    static constexpr std::size_t ALL_AVAILABLE = std::numeric_limits<std::size_t>::max();

    UriSet remove;
    UriSet create;

    auto now = Clock::now();
    GeneratePeerDeltas(create, remove);

    auto ptr = lane_identity_.lock();
    if (!ptr)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Lane identity not valid!");
    }

    for (auto &uri : create)
    {
      if ((currently_identifying.IsInFlight(uri)) ||
          (currently_laning.IsInFlight(uri)))
      {
        continue;
      }

      if (uri.scheme() == Uri::Scheme::Tcp)
      {
        auto const &peer = uri.AsPeer();

        try
        {
          shared_service_client_type conn =
              register_.CreateServiceClient<client_type>(manager_, peer.address(), peer.port());

          currently_identifying.Add(uri, IdentifyingConnection(conn, lane_identity_protocol_,
                                                               ptr->Identity()));
        }
        catch (std::exception &ex)
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "Error generated creating service client: ", ex.what());
        }
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Incorrect URI format");
      }
    }

    {
      currently_identifying.Resolve(now);

      for (auto &failure : currently_identifying.GetFailures(ALL_AVAILABLE))
      {
        Uri const &uri = failure.key;

        FETCH_LOG_WARN(LOGGING_NAME, "Identification failure on ", uri.uri());

        // get the underlying connection and close it
        auto conn = failure.promise.GetConn();
        conn->Close();
        conn.reset();
      }

      for (auto &success : currently_identifying.Get(ALL_AVAILABLE))
      {
        auto const &uri      = success.key;
        auto const &conn     = success.promised.first;
        auto const &identity = success.promised.second;

        auto details         = register_.GetDetails(conn->handle());
        details->is_outgoing = true;
        details->is_peer     = true;
        details->identity    = identity;

        currently_laning.Add(uri, LaningConnection(conn, lane_identity_protocol_));
      }
    }

    {
      currently_laning.Resolve(now);

      for (auto &failure : currently_laning.GetFailures(ALL_AVAILABLE))
      {
        Uri const &uri = failure.key;

        FETCH_LOG_WARN(LOGGING_NAME, "Lane connection failure on ", uri.uri());

        // get the underlying connection and close it
        auto conn = failure.promise.GetConn();
        conn->Close();
        conn.reset();
      }

      {
        FETCH_LOCK(services_mutex_);

        for (auto &success : currently_laning.Get(ALL_AVAILABLE))
        {
          auto const &uri  = success.key;
          auto const &conn = success.promised.first;

          peer_connections_[uri] = conn;
        }
      }
    }
  }

  void UseThesePeers(UriSet uris)
  {
    FETCH_LOCK(desired_connections_mutex_);
    desired_connections_ = std::move(uris);

    {
      FETCH_LOCK(services_mutex_);
      for (auto &peer_conn : peer_connections_)
      {
        if (desired_connections_.find(peer_conn.first) == desired_connections_.end())
        {
          FETCH_LOG_WARN(LOGGING_NAME, "DROP PEER: ", peer_conn.first.ToString());
        }
      }

      for (auto &uri : desired_connections_)
      {
        if (peer_connections_.find(uri) == peer_connections_.end())
        {
          FETCH_LOG_WARN(LOGGING_NAME, "ADD PEER: ", uri.ToString());
        }
      }
    }
  }

  void GeneratePeerDeltas(UriSet &create, UriSet &remove)
  {
    {
      FETCH_LOCK(desired_connections_mutex_);
      FETCH_LOCK(services_mutex_);

      // this method is the only one which needs both mutexes. It is
      // the moving interface between the DESIRED goal and the acting
      // to get closer to it. The mutexes are acquired in alphabetic
      // order.

      auto ident = lane_identity_.lock();
      if (!ident)
      {
        return;
      }

      for (auto &peer_conn : peer_connections_)
      {
        if (desired_connections_.find(peer_conn.first) == desired_connections_.end())
        {
          remove.insert(peer_conn.first);
        }
      }

      for (auto &uri : desired_connections_)
      {
        if (peer_connections_.find(uri) == peer_connections_.end())
        {
          create.insert(uri);
        }
      }
    }
  }

  /// @}
private:
  protocol_handler_type       lane_identity_protocol_;
  std::weak_ptr<LaneIdentity> lane_identity_;
  client_register_type        register_;
  network_manager_type        manager_;

  // Most methods do not need both mutexes. If they do, they should
  // acquire them in alphabetic order

  mutex::Mutex services_mutex_{__LINE__, __FILE__};
  mutex::Mutex desired_connections_mutex_{__LINE__, __FILE__};

  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
  std::vector<connection_handle_type>                                    inactive_services_;

  std::unordered_map<Uri, shared_service_client_type> peer_connections_;
  UriSet                                              desired_connections_;

  thread_pool_type thread_pool_;
};

}  // namespace ledger
}  // namespace fetch
