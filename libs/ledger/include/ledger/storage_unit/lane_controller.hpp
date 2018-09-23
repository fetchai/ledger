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
    thread_pool_->SetInterval(1000);
    thread_pool_->Start();
    thread_pool_->Post([this]() { thread_pool_->PostIdle([this]() { this->WorkCycle(); }); }, 1000);
  }

  /// External controls
  /// @{
  void RPCConnect(byte_array::ByteArray const &host, uint16_t const &port)
  {
    Connect(host, port);
  }

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

  using PingedPeer     = shared_service_client_type;
  using IdentifiedPeer = std::pair<shared_service_client_type, crypto::Identity>;
  using LanedPeer      = std::pair<shared_service_client_type, LaneIdentity::lane_type>;

  class PingingConnection : public network::ResolvableTo<PingedPeer>
  {
  public:
    static constexpr char const *LOGGING_NAME = "PingingConnection";

    using Timepoint = network::ResolvableTo<PingedPeer>::Timepoint;

    PingingConnection(shared_service_client_type conn, protocol_handler_type lane_identity_protocol)
    {
      attempts_               = 0;
      conn_                   = conn;
      lane_identity_protocol_ = lane_identity_protocol;
    }
    PingingConnection(const PingingConnection &other)
    {
      if (this != &other)
      {
        this->conn_                   = other.conn_;
        this->attempts_               = other.attempts_;
        this->timeout_                = other.timeout_;
        this->lane_identity_protocol_ = other.lane_identity_protocol_;
        this->prom_                   = other.prom_;
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

    State StartConnectionAttempt(const Timepoint &now)
    {
      attempts_++;
      FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:StartConnectionAttempt ", attempts_);
      auto p = conn_->Call(lane_identity_protocol_, LaneIdentityProtocol::PING);
      prom_.Adopt(p);
      timeout_.SetMilliseconds(now, 100);
      FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:CreatedPromise.. ", prom_.id());
      return State::WAITING;
    }

    virtual State GetState(Timepoint const &now) override
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState ");
      if (prom_.empty())
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState EMPTY");
        return StartConnectionAttempt(now);
      }

      switch (prom_.GetState())
      {
      case State::TIMEDOUT:  // should never see this.
      case State::WAITING:
        FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState == WAIT/TIME");
        if (timeout_.IsDue(now))
        {
          if (attempts_ >= 10)
          {
            FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState SET FAIL TIMEOUT");
            return State::TIMEDOUT;
          }
          FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState TRYAGAIN");
          return StartConnectionAttempt(now);
        }
        FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState WAITMORE");
        return State::WAITING;

      case State::FAILED:
        FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState FAILED");
        return State::FAILED;

      case State::SUCCESS:
        FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState SUCCESS");
        if (prom_.Get() == LaneIdentity::PING_MAGIC)
        {
          FETCH_LOG_DEBUG(LOGGING_NAME, "Workcycle:GetState MAGIC");
          return State::SUCCESS;
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Ping failed with wrong magic identifier");
          return State::FAILED;
        }
      }
    }
    virtual PromiseCounter id() const override
    {
      return prom_.id();
    }
    virtual PingedPeer Get() const override
    {
      return conn_;
    }

  private:
    network::PromiseOf<LaneIdentity::ping_type> prom_;
    shared_service_client_type                  conn_;
    network::FutureTimepoint                    timeout_;
    protocol_handler_type                       lane_identity_protocol_;
    int                                         attempts_;
  };

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

      case State::FAILED:
        return State::FAILED;

      case State::TIMEDOUT:
      case State::SUCCESS:
        return State::SUCCESS;
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

  using PingingPeers     = network::RequestingQueueOf<Uri, PingedPeer, PingingConnection>;
  using IdentifyingPeers = network::RequestingQueueOf<Uri, IdentifiedPeer, IdentifyingConnection>;
  using LaningPeers      = network::RequestingQueueOf<Uri, LanedPeer, LaningConnection>;

  PingingPeers     currently_pinging;
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
      if ((currently_pinging.IsInFlight(uri)) || (currently_identifying.IsInFlight(uri)) ||
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

          currently_pinging.Add(uri, PingingConnection(conn, lane_identity_protocol_));
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
      currently_pinging.Resolve(now);

      for (auto &failure : currently_pinging.GetFailures(ALL_AVAILABLE))
      {
        Uri const &uri = failure.key;

        FETCH_LOG_WARN(LOGGING_NAME, "Ping failure on ", uri.uri());

        // get the underlying connection and close it
        auto conn = failure.promise.GetConn();
        conn->Close();
        conn.reset();
      }

      for (auto &success : currently_pinging.Get(ALL_AVAILABLE))
      {
        auto const &uri = success.key;

        auto conn = success.promised;
        currently_identifying.Add(
            uri, IdentifyingConnection(conn, lane_identity_protocol_, ptr->Identity()));
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
    FETCH_LOCK(services_mutex_); // not ideal!
    desired_connections_ = std::move(uris);
  }

  void GeneratePeerDeltas(UriSet &create, UriSet &remove)
  {
    {
      FETCH_LOCK(services_mutex_);

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

  shared_service_client_type Connect(const Uri &uri)
  {
    if (uri.scheme() == Uri::Scheme::Tcp)
    {
      auto const &peer = uri.AsPeer();

      return Connect(peer.address(), peer.port());
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Incorrect URI format");
    }
    return shared_service_client_type();
  }

  shared_service_client_type Connect(byte_array::ByteArray const &host, uint16_t const &port)
  {
    std::string identifier = std::string(host);
    identifier += ":";
    identifier += std::to_string(port);
    FETCH_LOG_INFO(LOGGING_NAME, "Connecting to lane ", identifier);

    shared_service_client_type client =
        register_.CreateServiceClient<client_type>(manager_, host, port);

    auto ident = lane_identity_.lock();
    if (!ident)
    {
      // TODO(issue 7): Throw exception
      TODO_FAIL("Identity lost");
    }

    // Waiting for connection to be open

    using State = enum { WAITING, TIMEOUT, FAILED, OK };

    State r;

    for (int n = 0; n < 5; n++)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Trying to ping lane service @ ", ident, "    ", identifier,
                     " attempt ", n);
      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::PING);
      FETCH_LOG_PROMISE();
      r = WAITING;
      if (!p->Wait(100, false))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Response was failed promise");
        r = TIMEOUT;
      }
      else
      {
        if (p->As<LaneIdentity::ping_type>() == LaneIdentity::PING_MAGIC)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Response was PING_MAGIC");
          r = OK;
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Response was not a PING_MAGIC");
          r = FAILED;
        }
      }
      if (r == OK || r == FAILED)
      {
        break;
      }
    }

    switch (r)
    {
    case FAILED:
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Connection failed - closing in LaneController::Connect:1: == ", identifier);
      client->Close();
      client.reset();
      return nullptr;

    case WAITING:
    case TIMEOUT:
      FETCH_LOG_WARN(
          LOGGING_NAME,
          "Connection timed out - closing in LaneController::Connect:1: == ", identifier);
      client->Close();
      client.reset();
      return nullptr;

    case OK:
      break;
    }

    crypto::Identity peer_identity;
    {
      auto ptr = lane_identity_.lock();
      if (!ptr)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Lane identity not valid!");
        client->Close();
        client.reset();
        return nullptr;
      }

      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::HELLO, ptr->Identity());

      FETCH_LOG_PROMISE();
      if (!p->Wait(1000, true))  // TODO(issue 7): Make timeout configurable
      {
        FETCH_LOG_WARN(LOGGING_NAME,
                       "Connection timed out - closing in LaneController::Connect:2:");
        client->Close();
        client.reset();
        return nullptr;
      }
      p->As(peer_identity);
      FETCH_LOG_WARN(LOGGING_NAME, "Connection LaneController::Connect:2 IDENT EXCHANGED");
    }

    // Exchaning info
    auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::GET_LANE_NUMBER);

    FETCH_LOG_PROMISE();
    p->Wait(1000, true);  // TODO(issue 7): Make timeout configurable
    if (p->As<LaneIdentity::lane_type>() != ident->GetLaneNumber())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Could not connect to lane with different lane number: ",
                      p->As<LaneIdentity::lane_type>(), " vs ", ident->GetLaneNumber());
      client->Close();
      client.reset();
      // TODO(issue 11): Throw exception
      return nullptr;
    }

    {
      FETCH_LOCK(services_mutex_);
      services_[client->handle()] = client;
    }

    // Setting up details such that the rest of the lane what kind of
    // connection we are dealing with.
    auto details = register_.GetDetails(client->handle());

    details->is_outgoing = true;
    details->is_peer     = true;
    details->identity    = peer_identity;

    FETCH_LOG_INFO(LOGGING_NAME,
                   "Remote identity: ", byte_array::ToBase64(peer_identity.identifier()));
    return client;
  }

  /// @}
private:
  protocol_handler_type       lane_identity_protocol_;
  std::weak_ptr<LaneIdentity> lane_identity_;
  client_register_type        register_;
  network_manager_type        manager_;

  mutex::Mutex services_mutex_{__LINE__, __FILE__};
  std::unordered_map<connection_handle_type, shared_service_client_type> services_;
  std::vector<connection_handle_type>                                    inactive_services_;

  std::unordered_map<Uri, shared_service_client_type> peer_connections_;
  UriSet                                              desired_connections_;

  thread_pool_type thread_pool_;
};

}  // namespace ledger
}  // namespace fetch
