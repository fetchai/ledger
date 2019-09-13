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

#include "core/mutex.hpp"
#include "network/uri.hpp"
#include "network/management/network_manager.hpp"
#include "network/tcp/tcp_client.hpp"
#include "network/tcp/tcp_server.hpp"

#include <memory>
#include <queue>
#include <vector>


namespace fetch {
namespace dmlf {

uint16_t ephem_port_();

class MuddleLearnerNetworker: public ILearnerNetworker
{
public:
  using Intermediate = byte_array::ByteArray;
  using IUpdatePtr   = std::shared_ptr<IUpdate>;
  using QueueUpdates = std::priority_queue<IUpdatePtr, std::vector<IUpdatePtr>, std::greater<IUpdatePtr>>;
  using Uri          = fetch::network::Uri;
  
  MuddleLearnerNetworker(std::vector<Uri> peers, std::shared_ptr<network::NetworkManager> nnmm = nullptr)
  : nm_{nnmm}, nm_mine_{false}
  , initial_peers_{peers}
  , listen_port_{ephem_port_()}
  {
    if(!nm_)
    {
      nm_ = std::make_shared<network::NetworkManager>("dmlf", 4);
      nm_mine_ = true;
    }
    
    upds_out_ = std::make_shared<TCPServer>(listen_port_,*nm_);
    std::cout << "Listening on port " << listen_port_ << std::endl;
    
    for (auto& uri : initial_peers_)
    {
      auto conn = std::make_shared<TCPClient>(*nm_);
      conn->Connect(uri.AsPeer().address(), uri.AsPeer().port());
      upds_in_.push_back(conn);
      std::cout << "Added connection to " << uri.AsPeer().address() << ":" << uri.AsPeer().port() << std::endl;
    }
  }
  
  virtual ~MuddleLearnerNetworker()
  {
  }
  
  void start();

  virtual void pushUpdate( std::shared_ptr<IUpdate> update) override;
  virtual std::size_t getUpdateCount() const override;
  virtual std::shared_ptr<IUpdate> getUpdate() override;
  virtual std::size_t getCount() override;

protected:
  virtual Intermediate getUpdateIntermediate();

private:
  using Mutex     = fetch::Mutex;
  using Lock      = std::unique_lock<Mutex>;
  using TCPServer = fetch::network::TCPServer;
  using TCPClient = fetch::network::TCPClient;

  QueueUpdates  updates_;
  mutable Mutex updates_m_; 
  
  std::shared_ptr<network::NetworkManager> nm_;
  bool nm_mine_;
  
  std::vector<Uri> initial_peers_;
  uint16_t listen_port_;

  std::shared_ptr<TCPServer> upds_out_;
  std::vector<std::shared_ptr<TCPClient>> upds_in_;

  MuddleLearnerNetworker(const MuddleLearnerNetworker &other) = delete;
  MuddleLearnerNetworker &operator=(const MuddleLearnerNetworker &other)  = delete;
  bool                    operator==(const MuddleLearnerNetworker &other) = delete;
  bool                    operator<(const MuddleLearnerNetworker &other)  = delete;
};

}  // namespace dmlf
}  // namespace fetch
