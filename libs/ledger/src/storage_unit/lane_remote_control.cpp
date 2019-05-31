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

#include "ledger/storage_unit/lane_remote_control.hpp"

#include "core/service_ids.hpp"
#include "ledger/storage_unit/lane_controller_protocol.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"

#include <stdexcept>

using fetch::muddle::MuddleEndpoint;

namespace fetch {
namespace ledger {
namespace {

using AddressList = std::vector<MuddleEndpoint::Address>;

AddressList GenerateAddressList(ShardConfigs const &shards)
{
  AddressList addresses{};
  addresses.reserve(shards.size());

  for (auto const &shard : shards)
  {
    addresses.emplace_back(shard.internal_identity->identity().identifier());
  }

  return addresses;
}

}  // namespace

LaneRemoteControl::LaneRemoteControl(MuddleEndpoint &endpoint, ShardConfigs const &shards,
                                     uint32_t log2_num_lanes)
  : addresses_(GenerateAddressList(shards))
  , rpc_client_("SADM", endpoint, SERVICE_LANE_CTRL, CHANNEL_RPC)
{
  uint32_t const num_lanes = 1u << log2_num_lanes;

  if (num_lanes != shards.size())
  {
    throw std::logic_error("Mismatch on the number of shard configurations");
  }
}

void LaneRemoteControl::UseThesePeers(LaneIndex lane, std::unordered_set<Uri> const &uris)
{
  try
  {
    // make the request to the RPC server
    auto p = rpc_client_.CallSpecificAddress(LookupAddress(lane), RPC_CONTROLLER,
                                             LaneControllerProtocol::USE_THESE_PEERS, uris);

    // wait for the response
    p->Wait();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to execute UserThesePeers");
  }
}

void LaneRemoteControl::Shutdown(LaneIndex lane)
{
  try
  {
    auto p = rpc_client_.CallSpecificAddress(LookupAddress(lane), RPC_CONTROLLER,
                                             LaneControllerProtocol::SHUTDOWN);

    FETCH_LOG_PROMISE();
    p->Wait();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to execute Shutdown");
  }
}

uint32_t LaneRemoteControl::GetLaneNumber(LaneIndex lane)
{
  auto p = rpc_client_.CallSpecificAddress(LookupAddress(lane), RPC_IDENTITY,
                                           LaneIdentityProtocol::GET_LANE_NUMBER);
  return p->As<uint32_t>();
}

int LaneRemoteControl::IncomingPeers(LaneIndex lane)
{
  int peers{-1};

  try
  {
    auto p = rpc_client_.CallSpecificAddress(LookupAddress(lane), RPC_CONTROLLER,
                                             LaneControllerProtocol::INCOMING_PEERS);
    peers  = p->As<int>();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to execute IncomingPeers");
  }

  return peers;
}

int LaneRemoteControl::OutgoingPeers(LaneIndex lane)
{
  int peers{-1};

  try
  {
    auto p = rpc_client_.CallSpecificAddress(LookupAddress(lane), RPC_CONTROLLER,
                                             LaneControllerProtocol::OUTGOING_PEERS);
    peers  = p->As<int>();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to execute IncomingPeers: ", ex.what());
  }

  return peers;
}

bool LaneRemoteControl::IsAlive(LaneIndex)
{
  return true;
}

LaneRemoteControl::Address const &LaneRemoteControl::LookupAddress(LaneIndex lane) const
{
  return addresses_.at(lane);
}

}  // namespace ledger
}  // namespace fetch
