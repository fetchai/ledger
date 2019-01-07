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

#include "bootstrap_monitor.hpp"
#include "fetch_version.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include <chrono>
#include <sstream>

namespace fetch {
namespace {

using variant::Variant;
using variant::Extract;
using network::Uri;
using http::JsonHttpClient;

const char *               BOOTSTRAP_HOST = "bootstrap.economicagents.com";
const uint16_t             BOOTSTRAP_PORT = 80;
const std::chrono::seconds UPDATE_INTERVAL{30};
constexpr char const *     LOGGING_NAME = "bootstrap";

}  // namespace

bool BootstrapMonitor::Start(UriList &peers)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Bootstrapping network node ", BOOTSTRAP_HOST, ':', BOOTSTRAP_PORT);

  // query our external address
  if (!UpdateExternalAddress())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to determine external address");
    return false;
  }

  // register the node with the bootstrapper
  if (!RegisterNode())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to register with the bootstrap server");
    return false;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Registered node with bootstrap network");

  // request the peers list
  if (!RequestPeerList(peers))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to request the peers from the bootstrap server");
    return false;
  }

  running_        = true;
  monitor_thread_ = std::make_unique<std::thread>(&BootstrapMonitor::ThreadEntryPoint, this);

  FETCH_LOG_INFO(LOGGING_NAME, "Bootstrapping network node...complete");

  return true;
}

void BootstrapMonitor::Stop()
{
  running_ = false;
  if (monitor_thread_)
  {
    monitor_thread_->join();
    monitor_thread_.reset();
  }
}

bool BootstrapMonitor::UpdateExternalAddress()
{
  bool success = false;

  JsonHttpClient ipify_client("api.ipify.org");

  Variant response;
  if (ipify_client.Get("/?format=json", response))
  {
    auto const &ip_address = response["ip"];

    if (ip_address.Is<std::string>())
    {
      external_address_ = ip_address.As<std::string>();
      FETCH_LOG_INFO(LOGGING_NAME, "Detected external address as: ", external_address_);

      success = true;
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid format of response");
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to query the IPIFY");
  }

  return success;
}

bool BootstrapMonitor::RequestPeerList(UriList &peers)
{
  // make the response
  std::ostringstream oss;
  oss << "/api/networks/" << network_id_ << "/discovery/";

  JsonHttpClient client{BOOTSTRAP_HOST, BOOTSTRAP_PORT};

  Variant request       = Variant::Object();
  request["public_key"] = byte_array::ToBase64(identity_.identifier());
  request["host"]       = external_address_;
  request["port"]       = port_;

  JsonHttpClient::Headers headers;
  headers["Authorization"] = "Token " + token_;

  Variant response;
  if (client.Post(oss.str(), headers, request, response))
  {
    // check the formatting
    if (!response.IsArray())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Incorrect peer-list formatting (array)");
      return false;
    }

    // loop through all the array alements
    for (std::size_t i = 0, size = response.size(); i < size; ++i)
    {
      auto const &peer_object = response[i];

      if (!peer_object.IsObject())
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Incorrect peer-list formatting (object)");
        return false;
      }

      // extract all the required fields
      std::string host;
      uint16_t    port = 0;
      if (Extract(peer_object, "host", host) && Extract(peer_object, "port", port))
      {
        std::string const uri = "tcp://" + host + ':' + std::to_string(port);
        peers.emplace_back(Uri{ConstByteArray{uri}});
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to extract data from object");
        return false;
      }
    }
  }

  return true;
}

bool BootstrapMonitor::RegisterNode()
{
  bool success = false;

  Variant request           = Variant::Object();
  request["public_key"]     = byte_array::ToBase64(identity_.identifier());
  request["network"]        = network_id_;
  request["host"]           = external_address_;
  request["port"]           = port_;
  request["client_name"]    = "constellation";
  request["client_version"] = fetch::version::FULL;
  request["host_name"]      = host_name_;

  Variant                 response;
  JsonHttpClient          client{BOOTSTRAP_HOST, BOOTSTRAP_PORT};
  JsonHttpClient::Headers headers;
  headers["Authorization"] = "Token " + token_;

  if (client.Post("/api/register/", headers, request, response))
  {
    success = true;
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Unable to make register call to bootstrap network");
  }

  return success;
}

bool BootstrapMonitor::NotifyNode()
{
  bool success = false;

  Variant request       = Variant::Object();
  request["public_key"] = byte_array::ToBase64(identity_.identifier());

  Variant                 response;
  JsonHttpClient          client{BOOTSTRAP_HOST, BOOTSTRAP_PORT};
  JsonHttpClient::Headers headers;
  headers["Authorization"] = "Token " + token_;

  if (client.Post("/api/notify/", headers, request, response))
  {
    success = true;
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Unable to make notify call to bootstrap server");
  }

  return success;
}

void BootstrapMonitor::ThreadEntryPoint()
{
  while (running_)
  {
    // periodically request peers, this allows the bootstrap node to see that we are still alive
    NotifyNode();

    std::this_thread::sleep_for(UPDATE_INTERVAL);
  }
}

}  // namespace fetch
