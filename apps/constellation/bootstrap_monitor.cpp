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

#include "bootstrap_monitor.hpp"
#include "ledger/chaincode/contract_context.hpp"
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

using variant::Extract;
using network::Uri;
using http::JsonClient;
using byte_array::ConstByteArray;

using StringSet = std::unordered_set<ConstByteArray>;

char const *               BOOTSTRAP_HOST = "https://bootstrap.fetch.ai";
const std::chrono::seconds UPDATE_INTERVAL{30};
constexpr char const *     LOGGING_NAME = "bootstrap";

StringSet const VALID_PARAMETERS{"-block-interval", "-lanes",       "-slices",
                                 "-experimental",   "-aeon-period", "-pos"};

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

  // signal that we want to have the V2 response from the server
  headers["Accept"] = "application/vnd.fetch.bootstrap.v2+json";

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

bool BootstrapMonitor::DiscoverPeers(DiscoveryResult &output, std::string const &external_address)
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
  if (!RunDiscovery(output))
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

    if (ip_address.IsString())
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

bool BootstrapMonitor::RunDiscovery(DiscoveryResult &output)
{
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
  request["component"]      = "ledger";
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

  if (!response.Has("result"))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (no result)");
    FETCH_LOG_WARN(LOGGING_NAME, "Server Response: ", response);
    return false;
  }

  auto const &result = response["result"];

  // payload detection
  if (result.IsArray())
  {
    if (!ParseDiscoveryV1(result, output))
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Malformed response from bootstrap server (unable to parse v1 response)");
      return false;
    }
  }
  else if (result.IsObject())
  {
    // parse the version number from the field
    int64_t version{0};
    if (!Extract(result, "version", version))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (no version field)");
      return false;
    }

    if (version == 2)
    {
      if (!ParseDiscoveryV2(result, output))
      {
        FETCH_LOG_WARN(LOGGING_NAME,
                       "Malformed response from bootstrap server (can't parse V2 response)");
        return false;
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Malformed response from bootstrap server (version ", version,
                     " not supported)");
      return false;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Malformed response from bootstrap server (unable to identify payload)");
    return false;
  }

  // assume all goes well
  return true;
}

bool BootstrapMonitor::ParseDiscoveryV1(Variant const &arr, DiscoveryResult &result)
{
  return ParseNodeList(arr, result.uris);
}

bool BootstrapMonitor::ParseDiscoveryV2(Variant const &obj, DiscoveryResult &result)
{
  if (!obj.IsObject())
  {
    return false;
  }

  bool const all_fields_present = obj.Has("genesis") && obj.Has("nodes");
  if (!all_fields_present)
  {
    return false;
  }

  auto const &genesis = obj["genesis"];
  auto const &nodes   = obj["nodes"];

  bool const genesis_is_object = genesis.IsObject() && genesis.Has("contents") && genesis.Has("parameters");
  bool const all_sub_fields_present = nodes.IsArray() && (genesis.IsNull() || genesis_is_object);
  if (!all_sub_fields_present)
  {
    return false;
  }

  if (!ParseNodeList(nodes, result.uris))
  {
    return false;
  }

  // if no genesis configuration has been provided then use do not parse one
  if (genesis.IsNull())
  {
    return true;
  }

  // default case parse the genesis configuration
  return ParseGenesisConfiguration(genesis["contents"], result.genesis) &&
         ParseConfigurationUpdates(genesis["parameters"], result.config_updates);
}

bool BootstrapMonitor::ParseNodeList(Variant const &arr, UriSet &peers)
{
  if (!arr.IsArray())
  {
    return false;
  }

  // loop through all the results
  for (std::size_t i = 0, size = arr.size(); i < size; ++i)
  {
    auto const &peer_object = arr[i];

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
      return false;
    }

    std::string const uri_string = "tcp://" + host + ':' + std::to_string(port);

    // attempt to parse the URL being given
    if (!uri.Parse(uri_string))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse the URI: ", uri_string);
      return false;
    }

    peers.emplace(std::move(uri));
  }

  return true;
}

bool BootstrapMonitor::ParseGenesisConfiguration(Variant const &obj, std::string &genesis)
{
  if (!obj.IsObject())
  {
    return false;
  }

  try
  {
    std::ostringstream oss;
    oss << obj;

    genesis = oss.str();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to process genesis configuration: ", ex.what());
    return false;
  }

  return true;
}

bool BootstrapMonitor::ParseConfigurationUpdates(Variant const &obj, ConfigUpdates &updates)
{
  bool success{false};

  if (obj.IsObject())
  {
    success = true;

    obj.IterateObject([&updates, &success](ConstByteArray const &key, Variant const &value) {
      // the type of the value must be a string to be valid
      if (!value.IsString())
      {
        success = false;
        return false;
      }

      // the key of the parameter must be a part of the valid set
      if (VALID_PARAMETERS.find(key) == VALID_PARAMETERS.end())
      {
        success = false;
        return false;
      }

      // add the value to the configuration updates
      updates.emplace(key, value.As<std::string>());

      return true;
    });
  }
  else if (obj.IsNull())
  {
    // the configuration updates can be null to signal that no updates are required
    success = true;
  }

  // ensure if we are not succesful that any partial updates are not stored in the output variable
  if (!success)
  {
    FETCH_LOG_WARN(LOGGING_NAME,
                   "Failed to parse configuration updates section of bootstrap config");
    updates.clear();
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

char const *BootstrapMonitor::ToString(State state)
{
  char const *text = "Unknown";

  switch (state)
  {
  case State::Notify:
    text = "Notify";
    break;
  }

  return text;
}

std::string const &BootstrapMonitor::external_address() const
{
  return external_address_;
}

core::WeakRunnable BootstrapMonitor::GetWeakRunnable() const
{
  return state_machine_;
}

}  // namespace fetch
