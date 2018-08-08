#include "bootstrap_monitor.hpp"

#include <chrono>

namespace fetch {
namespace {

constexpr std::size_t BUFFER_SIZE = 1024;
constexpr char const *BOOTSTRAP_HOST = "35.189.67.157";
//constexpr char const *BOOTSTRAP_HOST = "127.0.0.1";
constexpr uint16_t BOOTSTRAP_PORT  = 10000;

const std::chrono::seconds UPDATE_INTERVAL{2};

} // namespace

bool BootstrapMonitor::Start(PeerList &peers)
{
  bool success = false;

  fetch::logger.Info("Bootstrapping network node ", BOOTSTRAP_HOST, ':', BOOTSTRAP_PORT);

  // request the peers list
  if (RequestPeerList(peers))
  {
    running_ = true;
    monitor_thread_ = std::make_unique<std::thread>(&BootstrapMonitor::ThreadEntryPoint, this);
    success = true;

    fetch::logger.Info("Bootstrapping network node...complete");
  }
  else
  {
    fetch::logger.Warn("Failed to request the peers from the bootstrap node");
  }

  std::this_thread::sleep_for(UPDATE_INTERVAL);

  return success;
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

bool BootstrapMonitor::RequestPeerList(BootstrapMonitor::PeerList &peers)
{
  // create the request
  fetch::script::Variant request;
  request.MakeObject();
  request["type"] = "peer-list";
  request["port"] = port_ + fetch::Constellation::P2P_PORT_OFFSET;
  request["network-id"] = network_id_;

  // make the response
  fetch::script::Variant response;
  if (MakeRequest(request, response))
  {
    if (!response.is_object())
    {
      fetch::logger.Warn("Incorrect peer-list formatting (object)");
      return false;
    }

    auto const &peer_list = response["peers"];
    if (peer_list.is_undefined())
    {
      fetch::logger.Warn("Incorrect peer-list formatting (undefined)");
      return false;
    }

    if (!peer_list.is_array())
    {
      fetch::logger.Warn("Incorrect peer-list formatting (array)");
      return false;
    }

    fetch::network::Peer peer;
    for (std::size_t i = 0; i < peer_list.size(); ++i)
    {
      std::string const peer_address = peer_list[i].As<std::string>();
      if (peer.Parse(peer_address))
      {
        peers.push_back(peer);
      }
      else
      {
        fetch::logger.Warn("Failed to parse address: ", peer_address);
      }
    }
  }

  return true;
}

bool BootstrapMonitor::MakeRequest(script::Variant const &request, script::Variant &response)
{
  // resolve the address of the bootstrap node
  Resolver::query query(BOOTSTRAP_HOST, std::to_string(BOOTSTRAP_PORT));
  Resolver::iterator endpoint = resolver_.resolve(query);

  std::error_code ec{};
  if (endpoint != Resolver::iterator{}) {

    // connect to the server
    socket_.connect(*endpoint, ec);
    if (ec)
    {
      fetch::logger.Warn("Failed to connect to boostrap node: ", ec.message());
      return false;
    }

    // format the request
    std::string request_data;
    {
      std::ostringstream oss;
      oss << request;
      request_data = oss.str();
    }

    // send the request to the server
    socket_.write_some(asio::buffer(request_data), ec);
    if (ec)
    {
      fetch::logger.Warn("Failed to send boostrap request: ", ec.message());
      return false;
    }

    // resize (if required) the size of the input buffer and await the server response
    buffer_.Resize(BUFFER_SIZE);
    std::size_t const num_bytes = socket_.read_some(asio::buffer(buffer_.pointer(), buffer_.size()), ec);
    if (ec)
    {
      fetch::logger.Warn("Failed to recv the response from the server: ", ec.message());
      return false;
    }

    // update the size of the buffer
    if (num_bytes > 0)
    {
      buffer_.Resize(num_bytes);
    }
    else
    {
      fetch::logger.Warn("Didn't recv any more data");
      return false;
    }

    // try and parse the json response
    fetch::json::JSONDocument doc;
    try
    {
      doc.Parse(buffer_);
    }
    catch (json::JSONParseException &)
    {
      fetch::logger.Warn("JSON parse exception");
      return false;
    }

    response = doc.root();
  }

  return true;
}

void BootstrapMonitor::ThreadEntryPoint()
{
  PeerList peers;
  while (running_)
  {
    // periodically request peers, this allows the bootstrap node to see that we are still alive
    RequestPeerList(peers);

    std::this_thread::sleep_for(UPDATE_INTERVAL);
  }
}

} // namespace fetch

