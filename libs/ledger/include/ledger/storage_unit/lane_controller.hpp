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

#include <utility>

#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "network/management/connection_register.hpp"
#include "network/p2pservice/p2p_peer_details.hpp"
#include "network/service/service_client.hpp"
#include "network/uri.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/requesting_queue.hpp"
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
  using Clock = std::chrono::steady_clock;
  using thread_pool_type           = network::ThreadPool;

  static constexpr char const *LOGGING_NAME = "LaneController";

  LaneController(protocol_handler_type lane_identity_protocol,
                 std::weak_ptr<LaneIdentity> identity, client_register_type reg,
                 network_manager_type const &nm)
    : lane_identity_protocol_(lane_identity_protocol)
    , lane_identity_(std::move(identity))
    , register_(std::move(reg))
    , manager_(nm)
  {
    thread_pool_ = network::MakeThreadPool(3);
    thread_pool_ -> SetInterval(1000);
    thread_pool_ -> Start();
    thread_pool_ -> Post([this](){
        thread_pool_ -> PostIdle( [this](){ this -> WorkCycle(); } );
      }, 1000);
    
  }

  /// External controls
  /// @{
  void RPCConnect(byte_array::ByteArray const &host, uint16_t const &port)
  {
    Connect(host, port);
  }

  void TryConnect(p2p::EntryPoint const &ep)
  {
    for (auto &h : ep.host)
    {
      FETCH_LOG_INFO(LOGGING_NAME,"Lane trying to connect to ", h, ":", ep.port);

      if (Connect(h, ep.port))
      {
        break;
      }
    }
  }

  void RPCConnectToURIs(const std::vector<Uri> &uris)
  {
    for(auto &thing : uris)
    {
      FETCH_LOG_INFO(LOGGING_NAME,"WILL ATTEMPT TO CONNECT TO: ", thing.ToString());
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
    std::lock_guard<mutex_type> lock_(services_mutex_);
    int                         incoming = 0;
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
    std::lock_guard<mutex_type> lock_(services_mutex_);
    int                         outgoing = 0;
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
    std::lock_guard<mutex_type> lock_(services_mutex_);
    return services_[n];
  }

  using PingedPeer = shared_service_client_type;
  using IdentifiedPeer = std::pair<shared_service_client_type, crypto::Identity>;
  using LanedPeer = std::pair<shared_service_client_type, LaneIdentity::lane_type>;

  class PingingConnection : public network::ResolvableTo<PingedPeer>
  {
  public:
    using Timepoint = network::ResolvableTo<PingedPeer>::Timepoint;

    PingingConnection(shared_service_client_type conn, protocol_handler_type lane_identity_protocol)
    {
      attempts_ = 0;
      conn_ = conn;
      lane_identity_protocol_ = lane_identity_protocol;
    }
    PingingConnection(const PingingConnection &other)
    {
      if (this != &other)
      {
        this -> conn_ = other . conn_;
        this -> attempts_ = other . attempts_;
        this -> timeout_ = other . timeout_;
        this -> lane_identity_protocol_ = other . lane_identity_protocol_;
        this -> prom_ = other . prom_;
      }
    }

    shared_service_client_type GetConn() { return conn_; }

    virtual State GetState() override
    {
      return GetState(Clock::now());
    }

    State StartConnectionAttempt(const Timepoint &now)
    {
      attempts_++;
      FETCH_LOG_WARN("PingingConnection","Workcycle:StartConnectionAttempt ", attempts_);
      auto p = conn_ -> Call(lane_identity_protocol_, LaneIdentityProtocol::PING);
      prom_ . Adopt( p );
      timeout_ . SetMilliseconds(now, 100);
      FETCH_LOG_WARN("PingingConnection","Workcycle:CreatedPromise.. ", prom_ . id());
      return State::WAITING;
    }

    virtual State GetState(const Timepoint &now) override
    {
      FETCH_LOG_WARN("PingingConnection","Workcycle:GetState ");
      if (prom_ . empty())
      {
        FETCH_LOG_WARN("PingingConnection","Workcycle:GetState EMPTY");
        return StartConnectionAttempt(now);
      }

      switch(prom_ . GetState())
      {
      case State::TIMEDOUT: // should never see this.
      case State::WAITING:
        FETCH_LOG_WARN("PingingConnection","Workcycle:GetState == WAIT/TIME");
        if (timeout_ . IsDue(now))
        {
          if (attempts_ >= 10)
          {
            FETCH_LOG_WARN("PingingConnection","Workcycle:GetState SET FAIL TIMEOUT");
            return State::TIMEDOUT;
          }
          FETCH_LOG_WARN("PingingConnection","Workcycle:GetState TRYAGAIN");
          return StartConnectionAttempt(now);
        }
        FETCH_LOG_WARN("PingingConnection","Workcycle:GetState WAITMORE");
        return State::WAITING;

      case State::FAILED:
        FETCH_LOG_WARN("PingingConnection","Workcycle:GetState FAILED");
        return State::FAILED;

      case State::SUCCESS:
        FETCH_LOG_WARN("PingingConnection","Workcycle:GetState SUCCESS");
        if ( prom_ . Get() == LaneIdentity::PING_MAGIC)
        {
          FETCH_LOG_WARN("PingingConnection","Workcycle:GetState MAGIC");
          return State::SUCCESS;
        }
        else
        {
          FETCH_LOG_WARN("PingingConnection","Workcycle:GetState NOMAGIC");
          return State::FAILED;
        }
      }
    }
    virtual PromiseCounter id() const override { return prom_ . id(); }
    virtual PingedPeer Get() const override
    {
      return conn_;
    }
  private:
    network::PromiseOf<LaneIdentity::ping_type> prom_;
    shared_service_client_type conn_;
    network::FutureTimepoint timeout_;
    protocol_handler_type lane_identity_protocol_;
    int attempts_;
  };

  class IdentifyingConnection : public network::ResolvableTo<IdentifiedPeer>
  {
  public:
    IdentifyingConnection(shared_service_client_type conn,
                          protocol_handler_type lane_identity_protocol,
                          crypto::Identity my_identity)
    {
      conn_ = conn;
      lane_identity_protocol_ = lane_identity_protocol;
      my_identity_ = my_identity;
    }
    IdentifyingConnection(const IdentifyingConnection &other)
    {
      if (this != &other)
      {
        this -> prom_ = other . prom_;
        this -> conn_ = other . conn_;
        this -> timeout_ = other . timeout_;
        this -> my_identity_ = other . my_identity_;
        this -> lane_identity_protocol_ = other . lane_identity_protocol_;
      }
    }

    shared_service_client_type GetConn() { return conn_; }

    virtual State GetState() override
    {
      return GetState(Clock::now());
    }

    virtual State GetState(const Timepoint &now) override
    {
      if (prom_ . empty())
      {
        auto p = conn_ -> Call(lane_identity_protocol_, LaneIdentityProtocol::HELLO, my_identity_);
        prom_ . Adopt( p );
        timeout_.SetMilliseconds(now, 100);
        return State::WAITING;
      }
      switch(prom_ . GetState())
      {
      case State::WAITING:
        if (timeout_ . IsDue(now))
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
    virtual PromiseCounter id() const override { return prom_ . id(); }
    virtual IdentifiedPeer Get() const override
    {
      return IdentifiedPeer(conn_, prom_ . Get());
    }
  private:
    network::PromiseOf<crypto::Identity> prom_;
    shared_service_client_type conn_;
    network::FutureTimepoint timeout_;
    protocol_handler_type lane_identity_protocol_;
    crypto::Identity my_identity_;
  };

  class LaningConnection : public network::ResolvableTo<LanedPeer>
  {
  public:
    LaningConnection(shared_service_client_type conn, protocol_handler_type lane_identity_protocol)
    {
      conn_ = conn;
      lane_identity_protocol_ = lane_identity_protocol;
    }
    LaningConnection(const LaningConnection &other)
    {
      if (this != &other)
      {
        this -> prom_ = other . prom_;
        this -> conn_ = other . conn_;
        this -> lane_identity_protocol_ = other . lane_identity_protocol_;
        this -> timeout_ = other . timeout_;
      }
    }

    shared_service_client_type GetConn() { return conn_; }

    virtual State GetState() override
    {
      return GetState(Clock::now());
    }

    virtual State GetState(const Timepoint &now) override
    {
      if (prom_ . empty())
      {
        auto p = conn_ -> Call(lane_identity_protocol_, LaneIdentityProtocol::GET_LANE_NUMBER);
        prom_ . Adopt( p );
        timeout_.SetMilliseconds(now, 100);
        return State::WAITING;
      }

      switch(prom_ . GetState())
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

    virtual PromiseCounter id() const override { return prom_ . id(); }
    virtual LanedPeer Get() const override
    {
      return LanedPeer(conn_, prom_ . Get());
    }
  private:
    network::PromiseOf<LaneIdentity::lane_type> prom_;
    shared_service_client_type conn_;
    protocol_handler_type lane_identity_protocol_;
    network::FutureTimepoint timeout_;
  };

  using PingingPeers = network::RequestingQueueOf<Uri, PingedPeer, PingingConnection>;
  using IdentifyingPeers =network:: RequestingQueueOf<Uri, IdentifiedPeer, IdentifyingConnection>;
  using LaningPeers = network::RequestingQueueOf<Uri, LanedPeer, LaningConnection>;

  PingingPeers currently_pinging_;
  IdentifyingPeers currently_identifying_;
  LaningPeers currently_laning_;


  void WorkCycle()
  {
    std::unordered_set<Uri> remove;
    std::unordered_set<Uri> create;

    auto now = Clock::now();
    GeneratePeerDeltas(create, remove);

    FETCH_LOG_WARN(LOGGING_NAME,"Lane Workcycle ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    FETCH_LOG_WARN(LOGGING_NAME,"Lane Workcycle remove ", remove.size());
    FETCH_LOG_WARN(LOGGING_NAME,"Lane Workcycle create ", create.size());

    auto ptr = lane_identity_.lock();
    if (!ptr)
    {
        FETCH_LOG_WARN(LOGGING_NAME,"Lane identity not valid!");
    }

    for(auto &uri : create)
    {
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: considering...", uri.ToString());
      if (
          (currently_pinging_ . Inflight(uri))
          ||
          (currently_identifying_ . Inflight(uri))
          ||
          (currently_laning_ . Inflight(uri))
          )
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: inflight...", uri.ToString());
        continue;
      }
      byte_array::ByteArray host;
      uint16_t port;
      if (uri.ParseTCP(host, port))
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: adding...", uri.ToString());
        try
        {
          shared_service_client_type conn =
            register_.CreateServiceClient<client_type>(manager_, host, port);
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: adding to pinger...", uri.ToString(), " --------------------- ", lane_identity_protocol_);
          currently_pinging_ . Add(uri, PingingConnection(conn, lane_identity_protocol_) );
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: added...", uri.ToString());
        }
        catch(...)
        {
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: adding... ERK!");
        }
      }
    }
    FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: pinging...");
    {
      auto r = currently_pinging_ . Resolve(now);
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: pinging... done:", std::get<0>(r));
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: pinging... fail:", std::get<1>(r));
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: pinging... wait:", std::get<2>(r));
      std::vector<PingingPeers::OutputType> successes;
      std::vector<PingingPeers::FailedOutputType> failures;
      do
      {
        currently_pinging_ . GetFailures(failures, 20);
        for(auto &failure : failures)
        {
          auto uri = failure . first;
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: pinging: Failed: ", uri.ToString());
          auto conn = failure . second . GetConn();
          conn -> Close();
          conn . reset();
        }
      }
      while(!failures.empty());

      do
      {
        currently_pinging_ . Get(successes, 20);
        for(auto &success : successes)
        {
          auto uri = success.first;
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: pinging: Success: ", uri.ToString());
          auto conn = success.second;
          currently_identifying_ . Add(uri, IdentifyingConnection(conn, lane_identity_protocol_, ptr->Identity()) );
        }
      }
      while(!successes.empty());
    }

    FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: identifying...");
    {
      auto r = currently_identifying_ . Resolve(now);
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: identifying... done:", std::get<0>(r));
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: identifying... fail:", std::get<1>(r));
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: identifying... wait:", std::get<2>(r));
      std::vector<IdentifyingPeers::OutputType> successes;
      std::vector<IdentifyingPeers::FailedOutputType> failures;
      do
      {
        currently_identifying_ . GetFailures(failures, 20);
        for(auto &failure : failures)
        {
          auto uri = failure . first;
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: identifying: Failed: ", uri.ToString());
          auto conn = failure . second . GetConn();
          conn -> Close();
          conn . reset();
        }
      }
      while(!failures.empty());

      do
      {
        currently_identifying_ . Get(successes, 20);
        for(auto &success : successes)
        {
          auto uri = success.first;
          auto conn = success.second.first;
          auto identity = success.second.second;

          auto details = register_.GetDetails(conn -> handle());
          details->is_outgoing = true;
          details->is_peer     = true;
          details->identity    = identity;

          currently_laning_ . Add(uri, LaningConnection(conn, lane_identity_protocol_) );
        }
      }
      while(!successes.empty());
    }
    FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: laning...");

    {
      auto r = currently_laning_ . Resolve(now);
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: laning... done:", std::get<0>(r));
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: laning... fail:", std::get<1>(r));
      FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: laning... wait:", std::get<2>(r));
      std::vector<LaningPeers::OutputType> successes;
      std::vector<LaningPeers::FailedOutputType> failures;
      do
      {
        currently_laning_ . GetFailures(failures, 20);
        for(auto &failure : failures)
        {
          auto uri = failure . first;
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: laning: Failed: ", uri.ToString());
          auto conn = failure . second . GetConn();
          conn -> Close();
          conn . reset();
        }
      }
      while(!failures.empty());

      std::lock_guard<mutex_type> lock_(services_mutex_);
      do
      {
        currently_laning_ . Get(successes, 20);
        for(auto &success : successes)
        {
          auto uri = success.first;
          auto conn = success.second.first;
          //auto lane_number = success.second.second;
          FETCH_LOG_WARN(LOGGING_NAME,"Workcycle: ESTABLISHED: ", uri.ToString());

          peer_connections_[uri] = conn;
        }
      }
      while(!successes.empty());
    }
    FETCH_LOG_WARN(LOGGING_NAME,"Lane Workcycle COMPLETED ----------------------------------------------------------------");
    return;
  }

  void UseThesePeers(std::set<Uri> uris)
  {
    desired_connections_ = uris;
  }

  void GeneratePeerDeltas(std::unordered_set<Uri> &create, std::unordered_set<Uri> &remove)
  {
    {
      std::lock_guard<mutex_type> lock_(services_mutex_);
      auto ident = lane_identity_.lock();
      if (!ident)
      {
        return;
      }
      for(auto& uri : desired_connections_)
      {
        FETCH_LOG_WARN(LOGGING_NAME, ident -> GetLaneNumber(), " -- UseThesePeers: ", uri.ToString());
      }

      for(auto &peer_conn : peer_connections_)
      {
        if (desired_connections_.find(peer_conn . first) == desired_connections_.end())
        {
          remove.insert(peer_conn . first);
        }
      }

      for(auto &uri : desired_connections_)
      {
        if (peer_connections_.find(uri) == peer_connections_.end())
        {
          create.insert(uri);
        }
      }
    }
  }
  //   for(auto &r : remove)
  //   {
  //     peer_connections_[r] -> Close();
  //     peer_connections_.erase(r);
  //   }

  //   for(auto &c : create)
  //   {
  //     auto conn = Connect(c);
  //     if (conn)
  //     {
  //       peer_connections_[c] = conn;
  //     }
  //   }
  // }

  
  shared_service_client_type Connect(const Uri &uri)
  {
    byte_array::ByteArray b;
    uint16_t port;
    if (uri.ParseTCP(b, port))
    {
      return Connect(b, port);
    }
    return shared_service_client_type();
  }

  shared_service_client_type Connect(byte_array::ByteArray const &host, uint16_t const &port)
  {
    std::string identifier = std::string(host);
    identifier += ":";
    identifier += std::to_string(port);
    FETCH_LOG_INFO(LOGGING_NAME,"Connecting to lane ", identifier);

    shared_service_client_type client =
        register_.CreateServiceClient<client_type>(manager_, host, port);

    auto ident = lane_identity_.lock();
    if (!ident)
    {
      // TODO(issue 7): Throw exception
      TODO_FAIL("Identity lost");
    }

    // Waiting for connection to be open

    using State = enum {
      WAITING, TIMEOUT, FAILED, OK
    };

    State r;

    for(int n = 0; n < 5; n++)
    {
      FETCH_LOG_INFO(LOGGING_NAME,"Trying to ping lane service @ ", ident, "    ", identifier, " attempt ", n);
      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::PING);
      FETCH_LOG_PROMISE();
      r = WAITING;
      if (!p->Wait(100, false))
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Response was failed promise");
        r = TIMEOUT;
      }
      else
      {
        if (p->As<LaneIdentity::ping_type>() == LaneIdentity::PING_MAGIC)
        {
          FETCH_LOG_WARN(LOGGING_NAME,"Response was PING_MAGIC");
          r = OK;
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME,"Response was not a PING_MAGIC");
          r = FAILED;
        }
      }
      if (r == OK || r == FAILED)
      {
        break;
      }
    }

    switch(r)
    {
    case FAILED:
      FETCH_LOG_WARN(LOGGING_NAME,"Connection failed - closing in LaneController::Connect:1: == ", identifier);
      client->Close();
      client.reset();
      return nullptr;

    case WAITING:
    case TIMEOUT:
      FETCH_LOG_WARN(LOGGING_NAME,"Connection timed out - closing in LaneController::Connect:1: == ", identifier);
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
        FETCH_LOG_WARN(LOGGING_NAME,"Lane identity not valid!");
        client->Close();
        client.reset();
        return nullptr;
      }

      auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::HELLO, ptr->Identity());

      FETCH_LOG_PROMISE();
      if (!p->Wait(1000, true))  // TODO(issue 7): Make timeout configurable
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Connection timed out - closing in LaneController::Connect:2:");
        client->Close();
        client.reset();
        return nullptr;
      }
      p->As(peer_identity);
      FETCH_LOG_WARN(LOGGING_NAME,"Connection LaneController::Connect:2 IDENT EXCHANGED");
    }

    // Exchaning info
    auto p = client->Call(lane_identity_protocol_, LaneIdentityProtocol::GET_LANE_NUMBER);

    FETCH_LOG_PROMISE();
    p->Wait(1000, true);  // TODO(issue 7): Make timeout configurable
    if (p->As<LaneIdentity::lane_type>() != ident->GetLaneNumber())
    {
      FETCH_LOG_ERROR(LOGGING_NAME,"Could not connect to lane with different lane number: ",
                   p->As<LaneIdentity::lane_type>(), " vs ", ident->GetLaneNumber());
      client->Close();
      client.reset();
      // TODO(issue 11): Throw exception
      return nullptr;
    }
    FETCH_LOG_WARN(LOGGING_NAME,"Connection LaneController::Connect:2 LANE NUMBER GOOD");

    {
      std::lock_guard<mutex_type> lock_(services_mutex_);
      services_[client->handle()] = client;
    }
    FETCH_LOG_WARN(LOGGING_NAME,"Connection LaneController::Connect:2 LANE NUMBER .. details?");

    // Setting up details such that the rest of the lane what kind of
    // connection we are dealing with.
    auto details = register_.GetDetails(client->handle());

    FETCH_LOG_WARN(LOGGING_NAME,"Connection LaneController::Connect:2 doing details");
    details->is_outgoing = true;
    details->is_peer     = true;
    details->identity    = peer_identity;

    FETCH_LOG_INFO(LOGGING_NAME,"Remote identity: ", byte_array::ToBase64(peer_identity.identifier()));
    FETCH_LOG_WARN(LOGGING_NAME,"Connection SUCCESS == ", identifier);
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

  std::map<Uri, shared_service_client_type> peer_connections_;
  std::set<Uri> desired_connections_;

  thread_pool_type     thread_pool_;
};

}  // namespace ledger
}  // namespace fetch
