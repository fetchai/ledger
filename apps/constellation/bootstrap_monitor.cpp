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
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"
#include "version/fetch_version.hpp"

#include <array>
#include <chrono>
#include <memory>
#include <random>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace {

using variant::Variant;
using variant::Extract;
using network::Uri;
using http::JsonClient;
using byte_array::ConstByteArray;

char const *               BOOTSTRAP_HOST = "https://bootstrap.fetch.ai";
const std::chrono::seconds UPDATE_INTERVAL{30};
constexpr char const *     LOGGING_NAME = "bootstrap";

/**
 * Helper object containing all the fields required to make the attestation
 */
struct Attestation
{
  ConstByteArray const public_key;
  ConstByteArray const nonce{GenerateNonce()};
  ConstByteArray const attestation;
  ConstByteArray const signature;

  /**
   * Build an attestation based on the a specified private/public key pair
   *
   * @param entity The private/public key pair to be used to signed the attestation
   */
  explicit Attestation(BootstrapMonitor::ProverPtr const &entity)
    : public_key{entity->identity().identifier()}
    , attestation{public_key + nonce}
    , signature{entity->Sign(attestation)}
  {}

  /**
   * Generate a random nonce that is used in combination of with the public key
   * @return The generated nonce bytes
   */
  static ConstByteArray GenerateNonce()
  {
    static constexpr std::size_t NUM_RANDOM_WORDS = 3;

    using Rng      = std::random_device;
    using RngWord  = Rng::result_type;
    using RngArray = std::array<RngWord, NUM_RANDOM_WORDS>;

    // generate the desired number of RNG words
    Rng      rng{};
    RngArray rng_words{};
    for (auto &word : rng_words)
    {
      word = rng();
    }

    return {reinterpret_cast<uint8_t const *>(rng_words.data()),
            rng_words.size() * sizeof(RngWord)};
  }
};

/**
 * Build a set of HTTP headers which will be used to making requests to be bootstrap server
 *
 * @param token The reference to the authorisation token (if any is present)
 * @return The generated set of headers
 */
http::JsonClient::Headers BuildHeaders(std::string const &token)
{
  JsonClient::Headers headers;

  if (!token.empty())
  {
    headers["Authorization"] = "Token " + token;
  }

  return headers;
}

}  // namespace

/**
 * Build a bootstrap monitor client
 *
 * @param entity The private/public key pair used to identify this node
 * @param p2p_port The listening p2p port
 * @param network_name The name of the network that the use wants to join
 * @param token The authorization token to join this network
 * @param host_name A identifiable name of the host
 */
BootstrapMonitor::BootstrapMonitor(ProverPtr entity, uint16_t p2p_port, std::string network_name,
                                   bool discoverable, std::string token, std::string host_name)
  : state_machine_{std::make_shared<StateMachine>("bootstrap", State::Notify,
                                                  BootstrapMonitor::ToString)}
  , entity_(std::move(entity))
  , network_name_(std::move(network_name))
  , discoverable_(discoverable)
  , port_(p2p_port)
  , host_name_(std::move(host_name))
  , token_(std::move(token))
{
  // register the state machine handlers
  state_machine_->RegisterHandler(State::Notify, this, &BootstrapMonitor::OnNotify);
}

bool BootstrapMonitor::DiscoverPeers(UriList &peers, std::string const &external_address)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Bootstrapping network node @ ", BOOTSTRAP_HOST);

  // query our external address if one has not been provided
  if (!external_address.empty())
  {
    external_address_ = external_address;
  }
  else if (!UpdateExternalAddress())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to determine external address");
    return false;
  }

  // request the peers list
  if (!RunDiscovery(peers))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to discover initial peers from the bootstrap server");
    return false;
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Bootstrapping network node...complete");

  return true;
}

bool BootstrapMonitor::UpdateExternalAddress()
{
  bool success = false;

  JsonClient ipify_client{JsonClient::ConnectionMode::HTTPS, "api.ipify.org"};

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

bool BootstrapMonitor::RunDiscovery(UriList &peers)
{
  bool success{false};

  // make the response
  std::ostringstream oss;
  oss << "/discovery/";

  // create the json client for this request
  JsonClient client = JsonClient::CreateFromUrl(BOOTSTRAP_HOST);

  // create the request
  Variant request = Variant::Object();

  // prepare the attestation to be sent in the request
  Attestation attestation{entity_};

  // populate the request
  request["network"]        = network_name_;
  request["public_key"]     = attestation.public_key.ToBase64();
  request["nonce"]          = attestation.nonce.ToBase64();
  request["signature"]      = attestation.signature.ToBase64();
  request["host"]           = external_address_;
  request["port"]           = port_;
  request["client_name"]    = "constellation";
  request["client_version"] = fetch::version::FULL;

  // add the optional host name if one is provided
  if (!host_name_.empty())
  {
    request["hostname"] = host_name_;
  }

  // add the the discoverable flag
  if (discoverable_)
  {
    request["discovery"] = "enabled";
  }

  Variant response;
  client.Post(oss.str(), BuildHeaders(token_), request, response);

  bool success_flag{false};
  if (!Extract(response, "success", success_flag))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (no success)");
    FETCH_LOG_WARN(LOGGING_NAME, "Server Response: ", response);
    return false;
  }

  if (!success_flag)
  {
    if (!response.Has("error"))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (no error)");
      FETCH_LOG_WARN(LOGGING_NAME, "Server Response: ", response);
      return false;
    }

    auto const &error_obj = response["error"];

    uint64_t    error_code{0};
    std::string message{};
    if (Extract(error_obj, "code", error_code) && Extract(error_obj, "message", message))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Error during bootstrap: ", message, " (", error_code, ")");
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (no msg, no code)");
      FETCH_LOG_WARN(LOGGING_NAME, "Server Response: ", response);
    }

    return false;
  }
  else
  {
    if (!response.Has("result"))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (no result)");
      FETCH_LOG_WARN(LOGGING_NAME, "Server Response: ", response);
      return false;
    }

    auto const &result = response["result"];
    if (!result.IsArray())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (result not array)");
      FETCH_LOG_WARN(LOGGING_NAME, "Server Response: ", response);
      return false;
    }

    // assume all goes well
    success = true;

    // loop through all the results
    for (std::size_t i = 0, size = result.size(); i < size; ++i)
    {
      auto const &peer_object = result[i];

      // formatting is correct check
      if (!peer_object.IsObject())
      {
        return false;
      }

      std::string host;
      uint16_t    port = 0;
      Uri         uri{};
      if (!(Extract(peer_object, "host", host) && Extract(peer_object, "port", port)))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (no host, no port)");
        FETCH_LOG_WARN(LOGGING_NAME, "Server Response: ", response);
        return false;
      }
      else
      {
        std::string const uri_string = "tcp://" + host + ':' + std::to_string(port);

        // attempt to parse the URL being given
        if (!uri.Parse(uri_string))
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse the URI: ", uri_string);
          return false;
        }

        peers.emplace_back(std::move(uri));
      }
    }
  }

  return success;
}

bool BootstrapMonitor::NotifyNode()
{
  bool success = false;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Notify bootstrap server...");

  Variant request = Variant::Object();

  // prepare the attestation to be sent in the request
  Attestation attestation{entity_};

  request["public_key"] = attestation.public_key.ToBase64();
  request["nonce"]      = attestation.nonce.ToBase64();
  request["signature"]  = attestation.signature.ToBase64();

  Variant    response;
  JsonClient client = JsonClient::CreateFromUrl(BOOTSTRAP_HOST);

  if (client.Post("/notify/", BuildHeaders(token_), request, response))
  {
    success = true;
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Unable to make notify call to bootstrap server");
  }

  return success;
}

BootstrapMonitor::State BootstrapMonitor::OnNotify()
{
  State next_state{State::Notify};

  // phone home to notify
  NotifyNode();

  // ensure there is a reasonable delay inbetween notifies
  state_machine_->Delay(UPDATE_INTERVAL);

  return next_state;
}

}  // namespace fetch
