#include "network/p2pservice/p2p_service.hpp"

namespace fetch {
namespace p2p {
namespace {

  template <typename T, typename H>
  bool Contains(std::unordered_set<T,H> const &set, T const &value)
  {
    return (set.find(value) != set.end());
  }

} // namespace

P2PService::P2PService(certificate_type &&certificate, uint16_t port,
                       fetch::network::NetworkManager const &tm)
  : super_type(port, tm), manager_(tm), certificate_(std::move(certificate))
{
  running_     = false;
  thread_pool_ = network::MakeThreadPool(1);
  FETCH_LOG_WARN(LOGGING_NAME, "Establishing P2P Service on rpc://0.0.0.0:", port);
  FETCH_LOG_WARN(LOGGING_NAME, "P2P Identity: ",
                 byte_array::ToBase64(certificate_->identity().identifier()));

  // Listening for new connections
  this->SetConnectionRegister(register_);

  // Identity
  identity_          = std::make_unique<P2PIdentity>(IDENTITY, register_, tm);
  my_details_        = identity_->my_details();
  identity_protocol_ = std::make_unique<P2PIdentityProtocol>(identity_.get());
  this->Add(IDENTITY, identity_protocol_.get());

  {
    EntryPoint discovery_ep;
    discovery_ep.identity     = certificate_->identity();
    discovery_ep.is_discovery = true;
    discovery_ep.port         = port;
    std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
    my_details_->details.entry_points.push_back(discovery_ep);
    my_details_->details.identity = certificate_->identity();

    my_details_->details.Sign(certificate_.get());
#if 0
    // TODO(issue 24): ECDSA verifier broke
    crypto::ECDSAVerifier verifier(certificate->identity());
    if(!my_details_->details.Verify(&verifier) ) {
      TODO_FAIL("Could not verify own identity");
    }
#endif
  }

  // P2P Peer Directory
  directory_ =
    std::make_unique<P2PPeerDirectory>(DIRECTORY, register_, thread_pool_, my_details_);
  directory_protocol_ = std::make_unique<P2PPeerDirectoryProtocol>(*directory_);
  this->Add(DIRECTORY, directory_protocol_.get());

  p2p_trust_.reset(new P2PTrust<byte_array::ConstByteArray>());
  // p2p_trust_protocol_.reset(new TrustModelProtocol(*directory_)); // TODO(kll)
  // this->Add(DIRECTORY, p2p_trust_protocol_.get()); // TODO(kll)

  // Adding hooks for listening to feeds etc
  register_.OnClientEnter(
    [](connection_handle_type const &i) { FETCH_LOG_INFO(LOGGING_NAME, "Peer joined: ", i); });

  register_.OnClientLeave(
    [](connection_handle_type const &i) { FETCH_LOG_INFO(LOGGING_NAME, "Peer left: ", i); });

  // TODO(issue 7): Get from settings
  min_connections_ = 2;
  max_connections_ = 3;
  tracking_peers_  = false;
}

void P2PService::Start()
{
  TCPServer::Start();

  thread_pool_->Start();
  directory_->Start();

  if (running_) return;
  running_ = true;

#if 0
  // Make the initial p2p connections
  // Note that we first connect after setting up the lanes to prevent that nodes
  // will be too fast in trying to set up lane connections.
  for (auto const &peer : initial_peers)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Connecting to ", peer.address(), ":", peer.port());

    Connect(peer.address(), peer.port());
  }
#endif

  NextServiceCycle();
}

void P2PService::Stop()
{
  if (!running_) return;
  running_ = false;

  TCPServer::Stop();

  directory_->Stop();
  thread_pool_->Stop();
}

P2PPeerDirectory::peer_details_map_type P2PService::SuggestPeersToConnectTo()
{
  return directory_->SuggestPeersToConnectTo();
}

bool P2PService::Connect(byte_array::ConstByteArray const &host, uint16_t const &port)
{
  FETCH_LOG_INFO(LOGGING_NAME, "START P2P CONNECT: Host: ", host, " port: ", port);

  shared_service_client_type client =
                               register_.CreateServiceClient<client_type>(manager_, host, port);

  LOG_STACK_TRACE_POINT;
  std::size_t n = 0;
  while ((n < 10) && (!client->is_alive()))
  {
    LOG_STACK_TRACE_POINT;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ++n;
  }

  if (n >= 10)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Connection never came to live in P2P module. Host: ", host,
                    " port: ", port);
    // TODO(issue 11): throw error?
    client.reset();
    return false;
  }

  // Getting own IP seen externally
  byte_array::ByteArray address;

  auto p = client->Call(IDENTITY, P2PIdentityProtocol::EXCHANGE_ADDRESS, host);
  /*
    if (!p.Wait(20))
  {
    FETCH_LOG_ERROR(LOGGING_NAME,"Connection doesn't seem to do IDENTITY");
    client.reset();
    return false;
  }
  */
  p->As(address);

  FETCH_LOG_INFO(LOGGING_NAME, "(Connect) Looks like my address is: ", address);

  {  // Exchanging identities including node setup
    LOG_STACK_TRACE_POINT;

    // Updating IP for P2P node
    for (auto &e : my_details_->details.entry_points)
    {
      if (e.is_discovery)
      {
        // TODO(issue 25): Make mechanim for verifying address
        std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
        e.host.insert(address);
      }
    }

    auto p = client->Call(IDENTITY, P2PIdentityProtocol::HELLO, my_details_->details);

    PeerDetails details = p->As<PeerDetails>();

    FETCH_LOG_INFO(LOGGING_NAME, "(Connect) The remote identity is: ",
                   byte_array::ToBase64(details.identity.identifier()));

    auto regdetails = register_.GetDetails(client->handle());

    {
      LOG_STACK_TRACE_POINT;
      std::lock_guard<mutex::Mutex> lock(*regdetails);

      regdetails->Update(details);
    }

    {
      std::lock_guard<mutex_type> lock_(peers_mutex_);
      peers_[client->handle()] = client;
      peer_identities_.emplace(details.identity.identifier());
    }
  }

  // Setup feeds to listen to
  directory_->ListenTo(client);

  FETCH_LOG_INFO(LOGGING_NAME, "END P2P CONNECT: Host: ", host, " port: ", port);

  return true;
}

void P2PService::AddLane(uint32_t const &lane, byte_array::ConstByteArray const &host,
                         uint16_t const &port,
                         crypto::Identity const &identity)
{
  // TODO(issue 24): connect

  {
    std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
    EntryPoint                    lane_details;
    lane_details.host.insert(host);
    lane_details.port     = port;
    lane_details.identity = identity;
    lane_details.lane_id  = lane;
    lane_details.is_lane  = true;
    my_details_->details.entry_points.push_back(lane_details);
  }

  identity_->MarkProfileAsUpdated();
}

void P2PService::AddMainChain(byte_array::ConstByteArray const &host, uint16_t const &port)
{

  // TODO(issue 24): connect
  {
    std::lock_guard<mutex::Mutex> lock(my_details_->mutex);
    EntryPoint                    mainchain_details;

    mainchain_details.host.insert(host);
    mainchain_details.port         = port;
    //     lane_details.public_key = "todo";
    mainchain_details.is_mainchain = true;
    my_details_->details.entry_points.push_back(mainchain_details);
  }

  identity_->MarkProfileAsUpdated();
}

void P2PService::NextServiceCycle()
{
  FETCH_LOG_INFO(LOGGING_NAME, "START SERVICE CYCLE");


  std::unordered_map<connected_identity_type, connection_handle_type> incoming_conns;

  connected_identities_type incoming;
  connected_identities_type outgoing;

  auto our_pk = certificate_->identity().identifier();

  std::vector<EntryPoint> orchestration;
  {
    LOG_STACK_TRACE_POINT;
    if (!running_) return;

    // Updating lists of incoming and outgoing
    using map_type = client_register_type::connection_map_type;

//      register_.WithConnections([this, &orchestration](map_type const &map) {
//        for (auto &c : map)

    register_.VisitConnections(
      [this, &orchestration, &incoming, &outgoing, &incoming_conns, our_pk](
        map_type::value_type const &c) {
        auto conn = c.second.lock();
        if (conn)
        {
          LOG_STACK_TRACE_POINT
          auto details = register_.GetDetails(conn->handle());

          std::lock_guard<mutex::Mutex> lock(*details);

          switch (conn->Type())
          {
            case network::AbstractConnection::TYPE_OUTGOING:
            {
              LOG_STACK_TRACE_POINT;
              generics::MilliTimer mt("network::AbstractConnection::TYPE_OUTGOING", 0);
              outgoing.insert(details->identity.identifier());
              for (auto &e : details->entry_points)
              {
                LOG_STACK_TRACE_POINT
#if 0
                char const *t = "unknown";
                if (e.is_lane)
                {
                  t = "lane ";
                }
                else if (e.is_mainchain)
                {
                  t = "disco";
                }
                else if (e.is_mainchain)
                {
                  t = "chain";
                }

                FETCH_LOG_INFO(LOGGING_NAME," - type: ", t, " port: ", e.port);
                for (auto const &h : e.host)
                {
                  FETCH_LOG_INFO(LOGGING_NAME,"   + host: ", h);
                }
#endif

                if (!e.was_promoted)
                {
                  orchestration.push_back(e);
                  e.was_promoted = true;
                }
              }
            }
              break;
            case network::AbstractConnection::TYPE_INCOMING:
//              FETCH_LOG_INFO(LOGGING_NAME, "Welcome: ", byte_array::ToBase64(details->identity.identifier()));

              // determine if this is a new identity

              // TODO(EJF): Might be inefficient / need to cache the right things
              if (details->identity.identifier() == our_pk)
              {
                FETCH_LOG_INFO(LOGGING_NAME, "Why am I connected to myself?");
              }

              incoming.insert(details->identity.identifier());
              incoming_conns[details->identity.identifier()] = c.first;

              break;
          }
        }
      });
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Incoming:");
  for (auto const &e : incoming)
  {
    FETCH_LOG_INFO(LOGGING_NAME, " - ", byte_array::ToBase64(e));
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Outgoing:");
  for (auto const &e : outgoing)
  {
    FETCH_LOG_INFO(LOGGING_NAME, " - ", byte_array::ToBase64(e));
  }

  std::vector<byte_array::ConstByteArray> new_incoming;
//    std::vector<byte_array::ConstByteArray> new_outgoing;

  {
    LOG_STACK_TRACE_POINT
    std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);

    // determine which of the incoming and outgoing connections are new
    std::set_difference(incoming.begin(), incoming.end(), incoming_.begin(), incoming_.end(),
                        std::back_inserter(new_incoming));
//      std::set_difference(outgoing.begin(), outgoing.end(), outgoing_.begin(), outgoing_.end(), std::back_inserter(new_incoming));

    while ((!new_incoming.empty()) && (incoming_.size() < max_connections_))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Welcome: ", byte_array::ToBase64(new_incoming.back()));

      incoming_.insert(new_incoming.back());
      new_incoming.pop_back();
    }

    outgoing_ = outgoing;
  }

  // close all the connections that we don't want
  for (auto &e : new_incoming)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Discarding connection: ", byte_array::ToBase64(e));

    auto it = incoming_conns.find(e);
    if (it == incoming_conns.end())
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Unable to locate handle for existing identity, something went wrpng");
    }

    // loolup the weak ptr to the connection in the register
    auto connection = register_.GetClient(it->second);
    if (connection)
    {
      connection->Close();
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Unalbe to lookup client from register");
    }
  }

  for (auto &e : orchestration)
  {
    thread_pool_->Post([this, e]() {
      if (!e.is_discovery)
      {
        if (callback_peer_update_profile_)
        {
          callback_peer_update_profile_(e);
        }
      }
    });
  }

#if 0
  thread_pool_->Post([this]() { this->NextServiceCycle(); },
                     1000);  // TODO(issue 7): add to config

#else
  thread_pool_->Post([this]() { this->ManageIncomingConnections(); },
                     1000);  // TODO(issue 7): add to config

#endif


  FETCH_LOG_INFO(LOGGING_NAME, "END SERVICE CYCLE");
}

void P2PService::ManageIncomingConnections()
{
  FETCH_LOG_INFO(LOGGING_NAME, "START MANAGE INCOMING");

  LOG_STACK_TRACE_POINT
  std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);

  // Timeout to send out a new tracking signal if needed
  // TODO(issue 7): Pull from settings
  {
    std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
    double                                ms  =
                                            double(
                                              std::chrono::duration_cast<std::chrono::milliseconds>(
                                                end - track_start_).count());
    if (ms > 5000) tracking_peers_ = false;
  }

  // Requesting new connections as needed
  if (incoming_.size() < min_connections_)
  {
    if (!tracking_peers_)
    {
      track_start_    = std::chrono::system_clock::now();
      tracking_peers_ = true;
      directory_->RequestPeersForThisNode();
    }
  }
  else if (incoming_.size() >= min_connections_)
  {
    if (tracking_peers_)
    {
      tracking_peers_ = false;
      directory_->EnoughPeersForThisNode();
    }
  }

  // Kicking random peers if needed
  if (incoming_.size() > max_connections_)
  {
    //std::cout << "Too many incoming connections" << std::endl;
    FETCH_LOG_INFO(LOGGING_NAME, "Too many incoming connections. ideal: ", max_connections_.load(),
                   " actual: ", incoming_.size());
    // TODO(issue 24):
  }

  thread_pool_->Post([this]() { this->ConnectToNewPeers(); });

  FETCH_LOG_INFO(LOGGING_NAME, "END MANAGE INCOMING");
}

void P2PService::ConnectToNewPeers()
{
  FETCH_LOG_INFO(LOGGING_NAME, "START CONNECT TO NEW PEERS");

  LOG_STACK_TRACE_POINT
  std::lock_guard<mutex::Mutex> lock(maintainance_mutex_);

  if (!running_) return;
  uint64_t create_count = 0;

  if (outgoing_.size() < min_connections_)
  {
    create_count = min_connections_ - outgoing_.size();
  }
  else if (outgoing_.size() < max_connections_)
  {
    create_count = 1;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "I need to connect to ", create_count, " peers");

  // TODO(EJF): This seems like the check should be done via the certificate
  auto my_pk = certificate_->identity().identifier();

  // Creating list of endpoints
  P2PPeerDirectory::peer_details_map_type suggest = directory_->SuggestPeersToConnectTo();
  std::vector<EntryPoint>                 endpoints;
  for (auto                               &s : suggest)
  {
    for (auto &e : s.second.entry_points)
    {
      // TODO(EJF): Check performed here (see above comment)
      if (e.identity.identifier() == my_pk) continue;

      if (e.is_discovery)
      {
        // skip the node if we already have an existing connection to this one
        if (Contains(incoming_, e.identity.identifier()) ||
            Contains(outgoing_, e.identity.identifier()) )
        {
          continue;
        }

        // add it to the possible list
        endpoints.push_back(e);
      }
    }
  }

  for (auto const &e : endpoints)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Suggested Peer: ", byte_array::ToBase64(e.identity.identifier()));
  }

  std::random_shuffle(endpoints.begin(), endpoints.end());

  // Connecting to peers who need connection
  for (auto const &e : endpoints)
  {
    if (create_count == 0) break;

    //      // we must be careful not to connect to peers to whom we have already connected
    //      bool already_connected = false;
    //      {
    //        std::lock_guard<mutex::Mutex> lock_peers(peers_mutex_);
    //        already_connected = peer_identities_.find(e.identity.identifier()) !=
    //        peer_identities_.end();
    //      }

    //      if (already_connected)
    //      {
    //        FETCH_LOG_INFO(LOGGING_NAME,"Discarding peer because already connected...");
    //        continue;
    //      }

    FETCH_LOG_INFO(LOGGING_NAME, "Trying connection to peer: ",
                   byte_array::ToBase64(e.identity.identifier()));

    TryConnect(e);
//      thread_pool_->Post([this, e]() { this->TryConnect(e); });
    --create_count;
  }

  // Connecting to other peers if needed
  // TODO(issue 24):

  thread_pool_->Post([this]() { this->NextServiceCycle(); });

  FETCH_LOG_INFO(LOGGING_NAME, "END CONNECT TO NEW PEERS");
}

void P2PService::TryConnect(EntryPoint const &e)
{
  FETCH_LOG_INFO(LOGGING_NAME, "#!# Trying to connect to ",
                 byte_array::ToBase64(e.identity.identifier()));

  if (e.identity.identifier() == "")
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Encountered empty identifier");
    return;
  }

  for (auto &h : e.host)
  {
    if (Connect(h, e.port)) break;
  }
}

} // namespace network
} // namespace fetch
