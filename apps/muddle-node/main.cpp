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

#include "core/commandline/parameter_parser.hpp"
#include "core/containers/set_difference.hpp"
#include "core/filesystem/read_file_contents.hpp"
#include "core/filesystem/write_to_file.hpp"
#include "core/macros.hpp"
#include "core/synchronisation/protected.hpp"
#include "crypto/ecdsa.hpp"
#include "http/module.hpp"
#include "http/server.hpp"
#include "muddle/muddle_endpoint.hpp"
#include "muddle/muddle_interface.hpp"
#include "network/management/network_manager.hpp"
#include "telemetry/registry.hpp"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <list>
#include <random>
#include <thread>

using namespace std::this_thread;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace fetch;
using namespace fetch::network;
using namespace fetch::muddle;

namespace {

using fetch::commandline::ParamsParser;

using AddressList   = std::list<Address>;
using AddressSet    = std::unordered_set<Address>;
using MsgCounters   = std::unordered_map<Address, std::size_t>;
using Clock         = std::chrono::steady_clock;
using Timepoint     = Clock::time_point;
using HttpServerPtr = std::unique_ptr<http::HTTPServer>;
using HttpModulePtr = std::unique_ptr<http::HTTPModule>;

struct AggregateData
{
  MsgCounters counters{};
  std::size_t total_messages{0};
};

using Statistics = fetch::Protected<AggregateData>;

std::atomic<bool>        global_active{true};
std::atomic<std::size_t> global_interrupt_count{0};

Statistics gStatistics;

constexpr uint16_t    SERVICE{1};
constexpr uint16_t    CHANNEL{1};
constexpr char const *LOGGING_NAME{"main"};

/**
 * The main interrupt handler for the application
 */
void InterruptHandler(int /*signal*/)
{
  std::size_t const interrupt_count = ++global_interrupt_count;

  if (interrupt_count > 1)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "User requests stop of service (count: ", interrupt_count, ")");
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "User requests stop of service");
  }

  // signal that the program should stop
  global_active = false;

  if (interrupt_count >= 3)
  {
    std::exit(1);
  }
}

ProverPtr RestoreOrCreateKey(ParamsParser const &params)
{
  std::string key_path{};
  bool const  key_path_exists = params.LookupParam("key", key_path);

  // load up a private key if one exists
  std::shared_ptr<fetch::crypto::ECDSASigner> certificate{};

  if (key_path_exists)
  {
    auto contents = fetch::core::ReadContentsOfFile(key_path.c_str());
    if (contents.size() == 32)
    {
      certificate = std::make_shared<fetch::crypto::ECDSASigner>(contents);
    }
  }

  // fallback, if no certificate is loaded, generate a new one
  if (!certificate)
  {
    // generate a new one
    certificate = std::make_shared<fetch::crypto::ECDSASigner>();
  }

  if (key_path_exists)
  {
    fetch::core::WriteToFile(key_path.c_str(), certificate->private_key());
  }

  return certificate;
}

AddressSet GenerateDesiredAddresses()
{
  std::vector<Address> addresses{};
  gStatistics.ApplyVoid([&](AggregateData const &data) {
    for (auto const &element : data.counters)
    {
      addresses.emplace_back(element.first);
    }
  });

  // seed our RNG
  std::random_device rd;
  std::mt19937       g(rd());
  std::shuffle(addresses.begin(), addresses.end(), g);

  // create the desired address set
  AddressSet desired{};
  for (std::size_t i = 0, end = std::min<std::size_t>(3u, addresses.size()); i < end; ++i)
  {
    desired.emplace(addresses[i]);
  }

  return desired;
}

class MetricsModule : public http::HTTPModule
{
public:
  MetricsModule()
  {
    Get("/metrics", "Metrics feed.", [](http::ViewParameters const &, http::HTTPRequest const &) {
      static auto const TXT_MIME_TYPE = http::mime_types::GetMimeTypeFromExtension(".txt");

      // collect up the generated metrics for the system
      std::ostringstream stream;
      fetch::telemetry::Registry::Instance().Collect(stream);

      return http::HTTPResponse(stream.str(), TXT_MIME_TYPE);
    });
  }
};

static const uint16_t INVALID_PORT = std::numeric_limits<uint16_t>::max();

}  // namespace

int main(int argc, char **argv)
{
  ParamsParser parser;
  parser.Parse(argc, argv);

  // restore or create the muddle cert.
  auto prover = RestoreOrCreateKey(parser);

  NetworkManager nm{"main", 1};
  nm.Start();

  // define the optional http interface
  HttpServerPtr  http{};
  HttpModulePtr  metrics{};
  uint16_t const http_port = parser.GetParam("http", INVALID_PORT);
  if (INVALID_PORT != http_port)
  {
    http = std::make_unique<http::HTTPServer>(nm);

    // add the metrics module
    metrics = std::make_unique<MetricsModule>();
    http->AddModule(*metrics);

    http->Start(http_port);
  }

  char const *external         = std::getenv("MUDDLE_EXTERNAL");
  char const *external_address = (external == nullptr) ? "127.0.0.1" : external;

  auto muddle = CreateMuddle("exmp", prover, nm, external_address);

  FETCH_LOG_INFO(LOGGING_NAME, "Muddle Node: ", muddle->GetAddress().ToBase64());

  // convert the parameters into peers
  MuddleInterface::Peers peers{};
  for (std::size_t i = 1; i < parser.arg_size(); ++i)
  {
    peers.emplace(parser.GetArg(i));
  }

  // lookup the endpoint
  auto &endpoint = muddle->GetEndpoint();
  auto  sub      = endpoint.Subscribe(SERVICE, CHANNEL);
  sub->SetMessageHandler([&endpoint](Address const &from, Packet::Payload const &payload) {
    FETCH_UNUSED(payload);

    // aggregate the statistics
    gStatistics.ApplyVoid([&](AggregateData &data) {
      ++(data.counters[from]);
      ++data.total_messages;

      if ((data.total_messages & 0x0Fu) == 0)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Message Summary: ", data.total_messages, " from ",
                       data.counters.size(),
                       " peers (connected: ", endpoint.GetDirectlyConnectedPeers().size(), ")");

        for (auto const &element : data.counters)
        {
          FETCH_LOG_INFO(LOGGING_NAME, " - ", element.first.ToBase64(), " : ", element.second);
        }

        FETCH_LOG_INFO(LOGGING_NAME, "---");
      }
    });
  });

  // start the muddle server listening on a random port and connected to the specified peers
  MuddleInterface::Ports server_ports{parser.GetParam<uint16_t>("port", 0)};
  muddle->Start(peers, server_ports);

  // register the signal handlers
  std::signal(SIGINT, InterruptHandler);
  std::signal(SIGTERM, InterruptHandler);

  Timepoint last_update = Clock::now();
  while (global_active)
  {
    endpoint.Broadcast(SERVICE, CHANNEL, "hello");

    auto const now   = Clock::now();
    auto const delta = now - last_update;

    if (delta > 20s)
    {
      // generate the desired peers
      auto const desired_peers = GenerateDesiredAddresses();
      auto const current_peers = muddle->GetRequestedPeers();
      auto const additions     = desired_peers - current_peers;
      auto const removals      = current_peers - desired_peers;

      FETCH_LOG_INFO(LOGGING_NAME, "Update Peers");
      for (auto const &address : desired_peers)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "To: ", address.ToBase64());
      }

      // ensure we try and connect to the requested addresses
      muddle->ConnectTo(additions);
      muddle->DisconnectFrom(removals);

      // reset the duraction
      last_update = now;
    }

    sleep_for(1s);
  }

  if (http)
  {
    http->Stop();
    http.reset();
  }

  muddle->Stop();
  nm.Stop();

  return EXIT_SUCCESS;
}
