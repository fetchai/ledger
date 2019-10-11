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

#include "gtest/gtest.h"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"
#include "crypto/ecdsa.hpp"

#include "dmlf/remote_execution_host.hpp"
#include "dmlf/remote_execution_client.hpp"
#include "dmlf/remote_execution_protocol.hpp"
#include "core/service_ids.hpp"

#include <thread>
#include <iostream>

namespace fetch {
namespace dmlf {
  using CertificatePtr    = fetch::muddle::ProverPtr;
  using NetworkManager =fetch::network::NetworkManager;
  using MuddlePtr = fetch::muddle::MuddlePtr;
  using RemoteExecutionProtocol = fetch::dmlf::RemoteExecutionProtocol; 
  using ExecutionInterfacePtr = ExecutionWorkload::ExecutionInterfacePtr;

  using Server = muddle::rpc::Server;
  
  CertificatePtr LoadIdentity(const std::string &privkey)
  {
    using Signer = fetch::crypto::ECDSASigner;
    // load the key
    auto signer = std::make_unique<Signer>();
    signer->Load(fetch::byte_array::FromBase64(privkey)); 
    return signer;
  }

  const char *SERVER_PRIV = "BEb+rF65Dg+59XQyKcu9HLl5tJc9wAZDX+V0ud07iDQ=";
  const char *SERVER_PUB  = "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==";
  const char *CLIENT_PRIV = "4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=";
  const char *CLIENT_PUB  = "646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==";

  const unsigned short int SERVER_PORT = 1766;
  const unsigned short int CLIENT_PORT = 1767;

  class ServerHalf
  {
  public:
    std::shared_ptr<NetworkManager>                 netm_;
    MuddlePtr                                       mud_;
    std::shared_ptr<RemoteExecutionProtocol> proto_;

    std::shared_ptr<RemoteExecutionHost> host_;
    ExecutionInterfacePtr exec;
 std::shared_ptr<Server>                         server_;
    ServerHalf()
    {
      auto ident = LoadIdentity(SERVER_PRIV);
      netm_ = std::make_shared<NetworkManager>("LrnrNet", 4);
      netm_->Start();
      mud_ = muddle::CreateMuddle("Test", ident, *(this->netm_), "127.0.0.1");
      host_ = std::make_shared<RemoteExecutionHost>(mud_, exec);
      mud_->SetPeerSelectionMode(muddle::PeerSelectionMode::KADEMLIA);
      mud_->Start({SERVER_PORT});

         proto_  = std::make_shared<RemoteExecutionProtocol>(*host_);
        server_ = std::make_shared<Server>(mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
        server_->Add(RPC_DMLF, proto_.get());
    }
  };

  class ClientHalf
  {
  public:
    std::shared_ptr<NetworkManager>                 netm_;
    MuddlePtr                                       mud_;
    std::shared_ptr<RemoteExecutionProtocol> proto_;

    std::shared_ptr<RemoteExecutionClient> client_;
    ExecutionInterfacePtr exec;
 std::shared_ptr<Server>                         server_;
    ClientHalf()
    {
      auto ident = LoadIdentity(CLIENT_PRIV);
      netm_ = std::make_shared<NetworkManager>("LrnrNet", 4);
      netm_->Start();
      mud_ = muddle::CreateMuddle("Test", ident, *(this->netm_), "127.0.0.1");
      client_ = std::make_shared<RemoteExecutionClient>(mud_, exec);
        mud_->SetPeerSelectionMode(muddle::PeerSelectionMode::KADEMLIA);
        std::string server = "tcp://127.0.0.1:";
        server += std::to_string(SERVER_PORT);

        mud_->Start({server}, {CLIENT_PORT});

        proto_  = std::make_shared<RemoteExecutionProtocol>(*client_);
        server_ = std::make_shared<Server>(mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
        server_->Add(RPC_DMLF, proto_.get());

    }
  };


  class MuddleLearnerNetworkerTests : public ::testing::Test
  {
  public:

    ServerHalf server;
    ClientHalf client;

    void SetUp() override
    {
    }
  };

    TEST_F(MuddleLearnerNetworkerTests, test1)
    {

      sleep(1);

      auto p1 = client . client_ -> CreateExecutable(SERVER_PUB, "exe1", "foo");

      sleep(1);
      std::cout << server . host_ -> ExecuteOneWorkload() << std::endl;

      //std::cout << p1.Get() << std::endl;
      
      EXPECT_EQ(1, 2);
    }
}
}
