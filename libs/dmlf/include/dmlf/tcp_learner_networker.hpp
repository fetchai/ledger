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

#include "dmlf/ilearner_networker.hpp"
#include "dmlf/iupdate.hpp"
#include "dmlf/update.hpp"

#include "core/mutex.hpp"
#include "network/uri.hpp"
#include "network/management/network_manager.hpp"
//#include "network/management/connection_register.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/tcp/tcp_server.hpp"

#include <memory>
#include <queue>
#include <vector>


namespace fetch {
namespace dmlf {

uint16_t ephem_port_();

class TcpLearnerNetworker: public ILearnerNetworker
{
  class DmlfTCPServer : public fetch::network::TCPServer 
  {
  public:
    DmlfTCPServer(uint16_t port, NetworkManagerType const &network_manager, TcpLearnerNetworker& learner)
    : TCPServer(port, network_manager)
    , learner_{learner}
    {
    }
    void PushRequest(ConnectionHandleType client, network::MessageType const &msg) override
    {
      std::cout << client << std::endl;
      learner_.on_new_update_(msg);
    }
    ~DmlfTCPServer() = default;
  private:
    TcpLearnerNetworker& learner_;
  };
 
public:
  using IUpdatePtr   = std::shared_ptr<IUpdate>;
  using QueueUpdates = std::priority_queue<IUpdatePtr, std::vector<IUpdatePtr>, std::greater<IUpdatePtr>>;
  using Uri          = fetch::network::Uri;
  
  TcpLearnerNetworker(std::vector<Uri> peers, std::shared_ptr<network::NetworkManager> nnmm = nullptr)
  : TcpLearnerNetworker(ephem_port_(), peers, nnmm)
  {
  }

  TcpLearnerNetworker(uint16_t port, std::vector<Uri> peers, std::shared_ptr<network::NetworkManager> nnmm = nullptr)
  : nm_{nnmm}, nm_mine_{false}
  , initial_peers_{peers}
  , listen_port_{port}
  {
    if(!nm_)
    {
      nm_ = std::make_shared<network::NetworkManager>("dmlf", 4);
      nm_mine_ = true;
    }
    
    server_ = std::make_shared<DmlfTCPServer>(listen_port_,*nm_, *this);
    std::cout << "Listening on port " << listen_port_ << std::endl;
    
    add_peers_(initial_peers_);
  }
  
  virtual ~TcpLearnerNetworker()
  {
  }
  
  void start();

  int PeersNumber()
  {
    return -1; 
  }

  virtual void pushUpdate( std::shared_ptr<IUpdate> update) override;
  virtual std::size_t getUpdateCount() const override;
  virtual std::size_t getPeerCount() const override;
  
  //virtual std::size_t getUpdateCount() const override;
  //virtual std::shared_ptr<IUpdate> getUpdate() override;
  //virtual std::size_t getCount() override;

protected:
  using Intermediate     = ILearnerNetworker::Intermediate;
  using IntermediateList = std::list<Intermediate>;

  virtual Intermediate               getUpdateIntermediate() override;
private:
  using Mutex     = fetch::Mutex;
  using Lock      = std::unique_lock<Mutex>;
  using TCPClient = fetch::network::TCPClient;
  using TCPServer = fetch::network::TCPServer;

  QueueUpdates  updates_;
  mutable Mutex updates_m_; 
  IntermediateList updates_bytes_;

  std::shared_ptr<network::NetworkManager> nm_;
  bool nm_mine_;
  
  std::vector<Uri> initial_peers_;
  uint16_t listen_port_;

  std::shared_ptr<TCPServer> server_;
  std::vector<std::shared_ptr<TCPClient>> clients_;

  void add_peers_(std::vector<Uri> peers)
  {
    for (auto& uri : peers)
    {
      add_peer_(uri);
    }
  }

  void add_peer_(Uri& uri)
  {
    auto conn = std::make_shared<TCPClient>(*nm_);
    conn->Connect(uri.GetTcpPeer().address(), uri.GetTcpPeer().port());
    conn->OnMessage([this](network::MessageType const &upd) {on_new_update_(upd);});
    clients_.push_back(conn);
    std::cout << "Added connection to " << uri.GetTcpPeer().address() << ":" << uri.GetTcpPeer().port() << std::endl;
  }

  void on_new_update_(network::MessageType const &serialized_update)
  {
    //auto update = std::make_shared<fetch::dmlf::Update<std::string>>();
    //update->deserialise(serialized_update);
    //std::cout << "Got update: " << update->TimeStamp() << std::endl;
    std::cout << "Got update: " << serialized_update << std::endl;
    {
      Lock lock{updates_m_};
      updates_bytes_.push_back(serialized_update);
    }
  }

  void broadcast_update_(std::shared_ptr<IUpdate> update)
  {
    auto upd_bytes = update->serialise();
    server_->Broadcast(upd_bytes);
    for (auto& upd_in : clients_)
    {
      upd_in->Send(upd_bytes);
    }
  }
  
  TcpLearnerNetworker(const TcpLearnerNetworker &other) = delete;
  TcpLearnerNetworker &operator=(const TcpLearnerNetworker &other) = delete;
  bool operator==(const TcpLearnerNetworker &other) = delete;
  bool operator<(const TcpLearnerNetworker &other) = delete;
};

}  // dmlf
}  // fetch
