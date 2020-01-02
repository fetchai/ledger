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

#include "core/service_ids.hpp"
#include "crypto/ecdsa.hpp"
#include "json/document.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/server.hpp"
#include "version/cli_header.hpp"

#include "dmlf/execution/basic_vm_engine.hpp"
#include "dmlf/execution/execution_result.hpp"
#include "dmlf/remote_execution_host.hpp"
#include "dmlf/remote_execution_protocol.hpp"

#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;

namespace {

using fetch::crypto::ECDSASigner;
using fetch::dmlf::BasicVmEngine;
using fetch::dmlf::RemoteExecutionHost;
using fetch::dmlf::RemoteExecutionProtocol;
using fetch::json::JSONDocument;
using fetch::muddle::MuddlePtr;
using fetch::muddle::rpc::Server;
using fetch::network::NetworkManager;
using fetch::network::Uri;

using CertificatePtr             = std::shared_ptr<ECDSASigner>;
using NetworkManagerPtr          = std::shared_ptr<NetworkManager>;
using RpcServerPtr               = std::unique_ptr<Server>;
using RemoteExecutionHostPtr     = std::unique_ptr<RemoteExecutionHost>;
using RemoteExecutionProtocolPtr = std::unique_ptr<RemoteExecutionProtocol>;

constexpr char const *LOGGING_NAME{"dmlf-etch-node"};
constexpr char const *NET_MANAGER_NAME{"LrnrNet"};
constexpr int const   NET_MANAGER_THREADS{4};
constexpr char const  MUDD_NET_ID[5]{"Test"};
constexpr char const *MUDD_ADDR{"127.0.0.1"};

CertificatePtr create_identity(std::string const &key)
{
  // load the key
  auto signer = std::make_shared<ECDSASigner>();
  signer->Load(fetch::byte_array::FromBase64(key));
  return signer;
}

}  // namespace

class DmlfEtchNode
{
public:
  explicit DmlfEtchNode(NetworkManagerPtr netman = NetworkManagerPtr{})
    : netm_{std::move(netman)}
    , running_{false}
  {
    if (!netm_)
    {
      netm_ = std::make_shared<NetworkManager>(NET_MANAGER_NAME, NET_MANAGER_THREADS);
    }
  }

  ~DmlfEtchNode()                         = default;
  DmlfEtchNode(DmlfEtchNode const &other) = delete;
  DmlfEtchNode(DmlfEtchNode &&other)      = delete;
  DmlfEtchNode &operator=(DmlfEtchNode const &other) = delete;

  bool Configure(std::string const &config)
  {
    try
    {
      JSONDocument doc{config};
      auto         node_config = doc.root()["node"];

      uri_  = Uri(node_config["uri"].As<std::string>());
      port_ = uri_.GetTcpPeer().port();
      key_  = node_config["key"].As<std::string>();
      cert_ = create_identity(key_);
      if (!cert_)
      {
        throw std::runtime_error{"Bad key string : " + key_};
      }
      return true;
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Exception: ", ex.what());
      return false;
    }
  }

  void Run()
  {
    netm_->Start();

    muddle_ = fetch::muddle::CreateMuddle(MUDD_NET_ID, cert_, *(this->netm_), MUDD_ADDR);
    host_   = std::make_unique<RemoteExecutionHost>(muddle_, std::make_shared<BasicVmEngine>());
    muddle_->SetTrackerConfiguration(fetch::muddle::TrackerConfiguration::AllOn());
    muddle_->Start({port_});

    protocol_ = std::make_unique<RemoteExecutionProtocol>(*host_);
    server_ =
        std::make_unique<Server>(muddle_->GetEndpoint(), fetch::SERVICE_DMLF, fetch::CHANNEL_RPC);
    server_->Add(fetch::RPC_DMLF, protocol_.get());

    running_ = true;
  }

  bool ProcessOne()
  {
    return host_->ExecuteOneWorkload();
  }

  bool Running()
  {
    return running_;
  }

private:
  Uri            uri_;
  uint16_t       port_;
  std::string    key_;
  CertificatePtr cert_;

  NetworkManagerPtr netm_;
  MuddlePtr         muddle_;
  RpcServerPtr      server_;

  RemoteExecutionHostPtr     host_;
  RemoteExecutionProtocolPtr protocol_;
  bool                       running_;
};

int main(int argc, char **argv)
{
  // version header
  fetch::version::DisplayCLIHeader("DMLF Etch Node");

  // TODO(LR) Add proper params parsing
  if (argc != 2)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Usage: ", argv[0], " <config-file>");
    return EXIT_FAILURE;
  }

  std::ifstream config_file{std::string{argv[1]}};
  if (!config_file.is_open())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Couldn't open configuration file ", std::string{argv[1]});
    return EXIT_FAILURE;
  }
  std::stringstream config{};
  config << config_file.rdbuf();

  DmlfEtchNode node{};
  if (!node.Configure(config.str()))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Bad configuration file ", std::string{argv[1]});
    return EXIT_FAILURE;
  }

  node.Run();

  FETCH_LOG_INFO(LOGGING_NAME, "Waiting for Workload ...");
  while (node.Running())
  {
    bool waiting_msg = false;
    while (node.ProcessOne())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Processing Workload ...");
      waiting_msg = true;
    }
    if (waiting_msg)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Waiting for Workload ...");
    }
    std::this_thread::sleep_for(500ms);
  }

  return EXIT_SUCCESS;
}
