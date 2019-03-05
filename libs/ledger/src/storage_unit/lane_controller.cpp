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

#include "core/service_ids.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "network/generics/future_timepoint.hpp"
#include "network/generics/requesting_queue.hpp"
#include "network/management/connection_register.hpp"
#include "network/muddle/muddle.hpp"
#include "network/muddle/rpc/client.hpp"
#include "network/muddle/rpc/server.hpp"
#include "network/service/service_client.hpp"
#include "network/uri.hpp"

#include "ledger/storage_unit/lane_controller.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace fetch {
namespace ledger {

LaneController::LaneController(std::weak_ptr<LaneIdentity> identity, MuddlePtr muddle)
  : lane_identity_(std::move(identity))
  , muddle_(std::move(muddle))
{}

void LaneController::WorkCycle()
{
  UriSet remove{};
  UriSet create{};

  GeneratePeerDeltas(create, remove);

  FETCH_LOG_WARN(LOGGING_NAME, "WorkCycle:create:", create.size());
  FETCH_LOG_WARN(LOGGING_NAME, "WorkCycle:remove:", remove.size());

  for (auto &uri : create)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "WorkCycle:creating:", uri.ToString());
    muddle_->AddPeer(Uri(uri.ToString()));
  }

  for (auto &uri : create)
  {
    Address target_address{};
    if (muddle_->UriToDirectAddress(uri, target_address))
    {
      peer_connections_[uri] = target_address;
    }
  }
}
/// External controls
/// @{
void LaneController::RPCConnectToURIs(const std::vector<Uri> &uris)
{
  for (auto const &uri : uris)
  {
    FETCH_LOG_VARIABLE(uri);
    FETCH_LOG_INFO(LOGGING_NAME, "WILL ATTEMPT TO CONNECT TO: ", uri.uri());
  }
}

void LaneController::Shutdown()
{
  TODO_FAIL("Needs to be implemented");
}

void LaneController::StartSync()
{
  TODO_FAIL("Needs to be implemented");
}

void LaneController::StopSync()
{
  TODO_FAIL("Needs to be implemented");
}

void LaneController::UseThesePeers(UriSet uris)
{
  FETCH_LOCK(desired_connections_mutex_);
  desired_connections_ = std::move(uris);

  {
    FETCH_LOCK(services_mutex_);
    for (auto &peer_conn : peer_connections_)
    {
      if (desired_connections_.find(peer_conn.first) == desired_connections_.end())
      {
        auto ident = lane_identity_.lock();
        if (ident)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "DROP PEER to lane ", ident->GetLaneNumber(), " : ",
                         peer_conn.first.ToString());
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "DROP PEER to lane :( : ", peer_conn.first.ToString());
        }
        muddle_->DropPeer(peer_conn.first);
      }
    }

    for (auto &uri : desired_connections_)
    {
      if (peer_connections_.find(uri) == peer_connections_.end())
      {
        auto ident = lane_identity_.lock();
        if (ident)
        {
          FETCH_LOG_DEBUG(LOGGING_NAME, "ADD PEER to lane ", ident->GetLaneNumber(), " : ",
                          uri.ToString());
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "ADD PEER to lane :( : ", uri.ToString());
        }
        muddle_->AddPeer(uri);
      }
    }
  }
}

LaneController::AddressSet LaneController::GetPeers()
{
  FETCH_LOCK(desired_connections_mutex_);

  auto const connections = muddle_->GetConnections(true);

  AddressSet addresses;
  for (auto &connection : connections)
  {
    addresses.insert(connection.first);
  }

  return addresses;
}

void LaneController::GeneratePeerDeltas(UriSet &create, UriSet &remove)
{
  FETCH_LOCK(desired_connections_mutex_);
  FETCH_LOCK(services_mutex_);

  // this method is the only one which needs both mutexes. It is
  // the moving interface between the DESIRED goal and the acting
  // to get closer to it. The mutexes are acquired in alphabetic
  // order.

  auto ident = lane_identity_.lock();
  if (!ident)
  {
    return;
  }

  for (auto &peer_conn : peer_connections_)
  {
    if (desired_connections_.find(peer_conn.first) == desired_connections_.end())
    {
      remove.insert(peer_conn.first);
    }
  }

  for (auto &uri : desired_connections_)
  {
    if (peer_connections_.find(uri) == peer_connections_.end())
    {
      create.insert(uri);
      FETCH_LOG_WARN(LOGGING_NAME, "Scheduling Connection To ", uri.ToString());
    }
  }
}

}  // namespace ledger
}  // namespace fetch