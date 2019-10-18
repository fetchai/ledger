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

#include "crypto/ecdsa.hpp"
#include "gtest/gtest.h"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/client.hpp"
#include "muddle/rpc/server.hpp"

#include "core/service_ids.hpp"
#include "dmlf/execution/execution_engine_interface.hpp"
#include "dmlf/execution/execution_result.hpp"
#include "dmlf/remote_execution_client.hpp"
#include "dmlf/remote_execution_host.hpp"
#include "dmlf/remote_execution_protocol.hpp"
#include <ctime>

#include <iostream>
#include <thread>

namespace fetch {
namespace dmlf {
using CertificatePtr              = fetch::muddle::ProverPtr;
using NetworkManager              = fetch::network::NetworkManager;
using MuddlePtr                   = fetch::muddle::MuddlePtr;
using RemoteExecutionProtocol     = fetch::dmlf::RemoteExecutionProtocol;
using ExecutionInterfacePtr       = std::shared_ptr<ExecutionInterface>;
using ExecutionEngineInterfacePtr = ExecutionWorkload::ExecutionInterfacePtr;

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
const char *SERVER_PUB =
    "rOA3MfBt0DdRtZRSo/gBFP2aD/YQTsd9lOh/Oc/Pzchrzz1wfhTUMpf9z8cc1kRltUpdlWznGzwroO8/rbdPXA==";
const char *CLIENT_PRIV = "4DW/sW8JLey8Z9nqi2yJJHaGzkLXIqaYc/fwHfK0w0Y=";
const char *CLIENT_PUB =
    "646y3U97FbC8Q5MYTO+elrKOFWsMqwqpRGieAC7G0qZUeRhJN+xESV/PJ4NeDXtkp6KkVLzoqRmNKTXshBIftA==";

unsigned short int server_port = 1766;
unsigned short int client_port = 1767;

class DummyExecutionInterface : public ExecutionEngineInterface
{
public:
  DummyExecutionInterface()           = default;
  ~DummyExecutionInterface() override = default;
  ExecutionResult CreateExecutable(Name const & /*execName*/,
                                   SourceFiles const & /*sources*/) override
  {
    return ExecutionResult::MakeSuccess();
  }
  ExecutionResult DeleteExecutable(Name const & /*execName*/) override
  {
    return ExecutionResult::MakeSuccess();
  }

  ExecutionResult CreateState(Name const & /*stateName*/) override
  {
    return ExecutionResult::MakeSuccess();
  }
  ExecutionResult CopyState(Name const & /*srcName*/, Name const & /*newName*/) override
  {
    return ExecutionResult::MakeSuccess();
  }
  ExecutionResult DeleteState(Name const & /*stateName*/) override
  {
    return ExecutionResult::MakeSuccess();
  }

  ExecutionResult Run(Name const & /*execName*/, Name const & /*stateName*/,
                      std::string const & /*entrypoint*/, Params /*params*/) override
  {
    return ExecutionResult::MakeIntegerResult(4);
  }
};

class ServerHalf
{
public:
  std::shared_ptr<NetworkManager>          netm_;
  MuddlePtr                                mud_;
  std::shared_ptr<RemoteExecutionProtocol> proto_;

  std::shared_ptr<RemoteExecutionHost> host_;
  ExecutionEngineInterfacePtr          exec_;
  std::shared_ptr<Server>              server_;
  explicit ServerHalf(ExecutionEngineInterfacePtr exec = ExecutionEngineInterfacePtr())
    : exec_(exec)
  {
    auto ident = LoadIdentity(SERVER_PRIV);
    netm_      = std::make_shared<NetworkManager>("LrnrNet", 4);
    netm_->Start();
    mud_ = muddle::CreateMuddle("Test", ident, *(this->netm_), "127.0.0.1");

    if (!exec_)
    {
      auto e = std::make_shared<DummyExecutionInterface>();
      exec_  = e;
    }

    host_ = std::make_shared<RemoteExecutionHost>(mud_, exec);
    mud_->SetPeerSelectionMode(muddle::PeerSelectionMode::KADEMLIA);
    mud_->Start({server_port});

    proto_  = std::make_shared<RemoteExecutionProtocol>(*host_);
    server_ = std::make_shared<Server>(mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
    server_->Add(RPC_DMLF, proto_.get());
  }
};

class ClientHalf
{
public:
  std::shared_ptr<NetworkManager>          netm_;
  MuddlePtr                                mud_;
  std::shared_ptr<RemoteExecutionProtocol> proto_;

  std::shared_ptr<RemoteExecutionClient> client_;
  ExecutionInterfacePtr                  exec;
  std::shared_ptr<Server>                server_;
  ClientHalf()
  {
    auto ident = LoadIdentity(CLIENT_PRIV);
    netm_      = std::make_shared<NetworkManager>("LrnrNet", 4);
    netm_->Start();
    mud_    = muddle::CreateMuddle("Test", ident, *(this->netm_), "127.0.0.1");
    client_ = std::make_shared<RemoteExecutionClient>(mud_, exec);
    mud_->SetPeerSelectionMode(muddle::PeerSelectionMode::KADEMLIA);
    std::string server = "tcp://127.0.0.1:";
    server += std::to_string(server_port);

    mud_->Start({server}, {client_port});

    proto_  = std::make_shared<RemoteExecutionProtocol>(*client_);
    server_ = std::make_shared<Server>(mud_->GetEndpoint(), SERVICE_DMLF, CHANNEL_RPC);
    server_->Add(RPC_DMLF, proto_.get());
  }
};

class MuddleLearnerNetworkerTests : public ::testing::Test
{
public:
  std::shared_ptr<DummyExecutionInterface> iface;
  std::shared_ptr<ServerHalf>              server;
  std::shared_ptr<ClientHalf>              client;

  void SetUp() override
  {
    srand((unsigned int)time(NULL));
    server_port = (unsigned short int)(rand() % 10000 + 10000);
    client_port = server_port + 1;

    usleep(100000);
    iface = std::make_shared<DummyExecutionInterface>();
    usleep(100000);
    server = std::make_shared<ServerHalf>(iface);
    usleep(100000);
    client = std::make_shared<ClientHalf>();
    usleep(100000);
  }
};

TEST_F(MuddleLearnerNetworkerTests, test1)
{
  usleep(100000);
  while (server->mud_->GetNumDirectlyConnectedPeers() < 1)
  {
    usleep(100000);
  }

  auto p1 = client->client_->CreateExecutable(SERVER_PUB, "exe1", {{"exe1.etch", "foo"}});
  auto p2 = client->client_->CreateState(SERVER_PUB, "state1");
  auto p3 = client->client_->Run(SERVER_PUB, "exe1", "state1", "dummy_func");

  int pending = 3;
  while (pending > 0)
  {
    usleep(100000);
    if (server->host_->ExecuteOneWorkload())
    {
      pending--;
    }
  }

  usleep(100000);
  p3.Wait();
  auto res = p3.Get();

  EXPECT_EQ(res.output().As<int>(), 4);
}
}  // namespace dmlf
}  // namespace fetch
