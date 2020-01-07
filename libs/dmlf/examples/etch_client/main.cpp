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
#include "moment/clocks.hpp"
#include "muddle/muddle_interface.hpp"
#include "muddle/rpc/server.hpp"
#include "version/cli_header.hpp"

#include "dmlf/execution/basic_vm_engine.hpp"
#include "dmlf/execution/execution_result.hpp"
#include "dmlf/execution/local_executor.hpp"
#include "dmlf/remote_execution_client.hpp"
#include "dmlf/remote_execution_protocol.hpp"

#include <chrono>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;

namespace {

using fetch::crypto::ECDSASigner;
using fetch::json::JSONDocument;
using fetch::muddle::MuddlePtr;
using fetch::muddle::rpc::Server;
using fetch::network::NetworkManager;
// using fetch::crypto::ECDSAVerifier;
// using fetch::crypto::Identity;
using fetch::dmlf::BasicVmEngine;
using fetch::dmlf::ExecutionResult;
using fetch::dmlf::LocalExecutor;
using fetch::dmlf::RemoteExecutionClient;
using fetch::dmlf::RemoteExecutionProtocol;

using CertificatePtr             = std::shared_ptr<ECDSASigner>;
using NetworkManagerPtr          = std::shared_ptr<NetworkManager>;
using RpcServerPtr               = std::unique_ptr<Server>;
using RemoteExecutionClientPtr   = std::unique_ptr<RemoteExecutionClient>;
using RemoteExecutionProtocolPtr = std::unique_ptr<RemoteExecutionProtocol>;
using PromiseOfResult            = fetch::dmlf::RemoteExecutionClient::PromiseOfResult;

constexpr char const *LOGGING_NAME{"dmlf-etch-client"};
constexpr char const *NET_MANAGER_NAME{"LrnrNet"};
constexpr int const   NET_MANAGER_THREADS{4};
constexpr char const  MUDD_NET_ID[5]{"Test"};
constexpr char const *MUDD_ADDR{"127.0.0.1"};

CertificatePtr create_identity(std::string const &key)
{
  auto signer = std::make_shared<ECDSASigner>();
  signer->Load(fetch::byte_array::FromBase64(key));
  return signer;
}

std::vector<ExecutionResult> WaitAll(std::vector<PromiseOfResult> const &promises)
{
  std::vector<ExecutionResult> results{};
  results.reserve(promises.size());
  for (auto &promise : promises)
  {
    results.emplace_back(ExecutionResult{});
    if (!promise.GetResult(results.back()))
    {
      results.pop_back();
    }
  }

  return results;
}

}  // namespace

class DmlfEtchClient
{
public:
  explicit DmlfEtchClient(NetworkManagerPtr netman = NetworkManagerPtr{})
    : netm_{std::move(netman)}
    , running_{false}
    , cross_check_{false}
  {
    if (!netm_)
    {
      netm_ = std::make_shared<NetworkManager>(NET_MANAGER_NAME, NET_MANAGER_THREADS);
    }
  }

  ~DmlfEtchClient()                           = default;
  DmlfEtchClient(DmlfEtchClient const &other) = delete;
  DmlfEtchClient(DmlfEtchClient &&other)      = delete;
  DmlfEtchClient &operator=(DmlfEtchClient const &other) = delete;

  bool Configure(std::string const &config)
  {
    try
    {
      JSONDocument doc{config};

      auto client_config = doc.root()["client"];
      key_               = client_config["key"].As<std::string>();
      cert_              = create_identity(key_);
      if (!cert_)
      {
        throw std::runtime_error{"Bad key string : " + key_};
      }
      pub_ = compute_pub_();

      auto nodes       = doc.root()["nodes"];
      auto nodes_count = nodes.size();
      for (size_t i = 0; i < nodes_count; ++i)
      {
        std::string uri = nodes[i]["uri"].As<std::string>();
        nodes_uris_.push_back(std::move(uri));
        std::string pub = nodes[i]["pub"].As<std::string>();
        // TODO(LR) how to check if a public key is valid
        /*
        if(!ECDSAVerifier{Identity{pub})
        {
          throw std::runtime_error{"Bad public key"+pub};
        }
        */
        nodes_pubs_.push_back(std::move(pub));
      }

      return true;
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Exception: ", ex.what());
      return false;
    }
  }

  std::vector<PromiseOfResult> Execute(std::string const &etch)
  {
    if (!running_)
    {
      start_();
    }

    std::vector<PromiseOfResult> result;

    if (cross_check_)
    {
      result = execute_on_all_(etch);
    }
    else
    {
      result = std::vector<PromiseOfResult>{execute_on_one_(etch)};
    }
    return result;
  }

private:
  std::string              key_;
  std::string              pub_;
  std::vector<std::string> nodes_uris_;
  std::vector<std::string> nodes_pubs_;
  CertificatePtr           cert_;

  NetworkManagerPtr netm_;
  MuddlePtr         muddle_;
  RpcServerPtr      server_;

  RemoteExecutionClientPtr   client_;
  RemoteExecutionProtocolPtr protocol_;

  bool running_;
  bool cross_check_;

  void start_()
  {
    netm_->Start();

    std::unordered_set<std::string> uris{nodes_uris_.begin(), nodes_uris_.end()};

    muddle_ = fetch::muddle::CreateMuddle(MUDD_NET_ID, cert_, *(this->netm_), MUDD_ADDR);
    client_ = std::make_unique<RemoteExecutionClient>(
        muddle_, std::make_shared<LocalExecutor>(std::make_shared<BasicVmEngine>()));
    muddle_->SetTrackerConfiguration(fetch::muddle::TrackerConfiguration::AllOn());
    muddle_->Start(uris, std::vector<unsigned short int>{});

    protocol_ = std::make_unique<RemoteExecutionProtocol>(*client_);
    server_ =
        std::make_unique<Server>(muddle_->GetEndpoint(), fetch::SERVICE_DMLF, fetch::CHANNEL_RPC);
    server_->Add(fetch::RPC_DMLF, protocol_.get());

    running_ = true;
  }

  PromiseOfResult execute_on_one_(std::string const &etch)
  {
    size_t      node_index   = random(0, nodes_pubs_.size() - 1);
    std::string node         = nodes_pubs_[node_index];
    auto        exec_promise = execute_on_(node, etch);
    return exec_promise;
  }

  std::vector<PromiseOfResult> execute_on_all_(std::string const &etch)
  {
    std::vector<PromiseOfResult> promises;
    for (auto &node : nodes_pubs_)
    {
      auto exec_promise = execute_on_(node, etch);
      promises.push_back(exec_promise);
    }
    return promises;
  }

  PromiseOfResult execute_on_(std::string const &node, std::string const &etch)
  {
    std::string call_id = generate_unique_id_();

    FETCH_LOG_INFO(LOGGING_NAME, "Creating executable ", call_id, " on node ", node);
    auto create_exec_prom = client_->CreateExecutable(node, call_id, {{"source.etch", etch}});

    {
      ExecutionResult result{};
      if (!(create_exec_prom.GetResult(result) && result.succeeded()))
      {
        return create_exec_prom;
      }
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Creating state ", call_id, " on node ", node);
    auto create_state_prom = register_state_(node, call_id);

    {
      ExecutionResult result{};
      if (!(create_state_prom.GetResult(result) && result.succeeded()))
      {
        return create_state_prom;
      }
    }

    auto execute_prom = client_->Run(node, call_id, call_id, "main", {});
    FETCH_LOG_INFO(LOGGING_NAME, "Run RPC submited to node ", node);

    return execute_prom;
  }

  PromiseOfResult register_state_(std::string const &node, std::string const &state_name)
  {
    return client_->CreateState(node, state_name);
  }

  PromiseOfResult delete_state_(std::string const &node, std::string const &state_name)
  {
    return client_->DeleteState(node, state_name);
  }

  std::string generate_unique_id_()
  {
    // get time since epoch
    auto time_since_epoch =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                  fetch::moment::GetClock("default")->Now().time_since_epoch())
                                  .count());

    // generate unique id
    std::string unique_id = pub_ + "_" + std::to_string(time_since_epoch);

    return unique_id;
  }

  std::string compute_pub_()
  {
    // get client public key
    auto key = std::make_unique<ECDSASigner>();
    key->Load(fetch::byte_array::FromBase64(key_));
    return static_cast<std::string>(key->public_key().ToBase64());
  }

  std::size_t random(std::size_t start, std::size_t end)
  {
    std::random_device rd;
    std::mt19937       eng(rd());

    if (end <= start)
    {
      return start;
    }

    std::uniform_int_distribution<std::size_t> distr(start, end);
    return distr(eng);
  }
};

int main(int argc, char **argv)
{
  // version header
  fetch::version::DisplayCLIHeader("DMLF Etch Client");

  // TODO(LR) set appropriate log levels
  /*
  fetch::SetGlobalLogLevel(fetch::LogLevel::ERROR);
  fetch::SetLogLevel(LOGGING_NAME, fetch::LogLevel::INFO);
  */

  // TODO(LR) Add proper params parsing
  if (argc != 3)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Usage: ", argv[0], " <config-file> <etch-file>");
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

  std::ifstream etch_file{std::string{argv[2]}};
  if (!etch_file.is_open())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Couldn't open Etch file ", std::string{argv[2]});
    return EXIT_FAILURE;
  }
  std::stringstream etch{};
  etch << etch_file.rdbuf();

  DmlfEtchClient client{};
  if (!client.Configure(config.str()))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Bad configuration file ", std::string{argv[1]});
    return EXIT_FAILURE;
  }

  std::vector<PromiseOfResult> promises = client.Execute(etch.str());

  std::vector<ExecutionResult> results = WaitAll(promises);

  bool        success = true;
  bool        similar = true;
  std::string console = results[0].console();
  for (auto &result : results)
  {
    success = success && result.succeeded();
    similar = similar && (console == result.console());
  }

  if (!success)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Run failed : \n", results[0].error().message());
    return EXIT_FAILURE;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Run successful!");
  if (!similar)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Run got different outputs from different nodes");
  }

  FETCH_LOG_INFO(LOGGING_NAME, "[output]\n", results[0].console());
  return EXIT_SUCCESS;
}
