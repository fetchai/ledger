#include "core/script/variant.hpp"
#include "bootstrap_monitor.hpp"

#include <chrono>
#include <sstream>

namespace fetch {
namespace {

using script::Variant;
using http::JsonHttpClient;

const char *BOOTSTRAP_HOST = "127.0.0.1";
const uint16_t BOOTSTRAP_PORT = 8000;
const std::chrono::seconds UPDATE_INTERVAL{10};

} // namespace

bool BootstrapMonitor::Start(PeerList &peers)
{
  fetch::logger.Info("Bootstrapping network node ", BOOTSTRAP_HOST, ':', BOOTSTRAP_PORT);

  // query our external address
  if (!UpdateExternalAddress())
  {
    fetch::logger.Warn("Failed to determine external address");
    return false;
  }

  // register the node with the bootstrapper
  if (!RegisterNode())
  {
    fetch::logger.Warn("Failed to register the bootstrap node");
    return false;
  }

  fetch::logger.Info("Registered node with bootstrap network");

  // request the peers list
  if (!RequestPeerList(peers))
  {
    fetch::logger.Warn("Failed to request the peers from the bootstrap node");
    return false;
  }

  running_ = true;
  monitor_thread_ = std::make_unique<std::thread>(&BootstrapMonitor::ThreadEntryPoint, this);

  fetch::logger.Info("Bootstrapping network node...complete");

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

    if (ip_address.is_string())
    {
      external_address_ = ip_address.As<std::string>();
      fetch::logger.Info("Detected external address as: ", external_address_);

      success = true;
    }
    else
    {
      fetch::logger.Warn("Invalid format of response");
    }
  }
  else
  {
    fetch::logger.Warn("Unable to query the IPIFY");
  }

  return success;
}

bool BootstrapMonitor::RequestPeerList(BootstrapMonitor::PeerList &peers)
{
  // make the response
  std::ostringstream oss;
  oss << "/api/networks/" << network_id_ << "/discovery/";

  JsonHttpClient client{BOOTSTRAP_HOST, BOOTSTRAP_PORT};

  Variant request;
  request.MakeObject();
  request["public_key"] = byte_array::ToBase64(identity_.identifier());
  request["host"] = external_address_;
  request["port"] = port_ + fetch::Constellation::P2P_PORT_OFFSET;

  Variant response;
  if (client.Post(oss.str(), request, response))
  {
    // check the formatting
    if (!response.is_array())
    {
      fetch::logger.Warn("Incorrect peer-list formatting (array)");
      return false;
    }

    // loop through all the array alements
    for (std::size_t i = 0, size = response.size(); i < size; ++i)
    {
      auto const &peer_object = response[i];

      if (!peer_object.is_object())
      {
        fetch::logger.Warn("Incorrect peer-list formatting (object)");
        return false;
      }

      // extract all the required fields
      std::string host;
      uint16_t port = 0;
      if (script::Extract(peer_object, "host", host) && script::Extract(peer_object, "port", port))
      {
        peers.emplace_back(std::move(host), port);
      }
      else
      {
        fetch::logger.Warn("Failed to extract data from object");
        return false;
      }
    }
  }

  return true;
}

bool BootstrapMonitor::RegisterNode()
{
  bool success = false;

  Variant request;
  request.MakeObject();
  request["public_key"] = byte_array::ToBase64(identity_.identifier());
  request["network"] = network_id_;
  request["host"] = external_address_;
  request["port"] = port_ + fetch::Constellation::P2P_PORT_OFFSET;
  request["client_name"] = "constellation";
  request["client_version"] = "v0.0.1";

  Variant response;
  JsonHttpClient client{BOOTSTRAP_HOST, BOOTSTRAP_PORT};
  if (client.Post("/api/register/", request, response))
  {
    success = true;
  }
  else
  {
    fetch::logger.Info("Unable to make register call to bootstrap network");
  }

  return success;
}

bool BootstrapMonitor::NotifyNode()
{
  bool success = false;

  Variant request;
  request.MakeObject();
  request["public_key"] = byte_array::ToBase64(identity_.identifier());

  Variant response;
  JsonHttpClient client{BOOTSTRAP_HOST, BOOTSTRAP_PORT};
  if (client.Post("/api/notify/", request, response))
  {
    success = true;
  }
  else
  {
    fetch::logger.Info("Unable to make notify call to bootstrap network");
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

} // namespace fetch

