#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/mutex.hpp"
#include "ledger/storage_unit/lane_controller_protocol.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/storage_unit_client.hpp"
#include "network/p2pservice/p2p_lane_management.hpp"
#include "network/service/service_client.hpp"

#include <unordered_map>
namespace fetch {
namespace ledger {

class LaneRemoteControl : public p2p::LaneManagement
{
public:
  static constexpr char const *LOGGING_NAME = "LaneRemoteControl";

  using Mutex                = mutex::Mutex;
  using Promise              = service::Promise;
  using StorageUnitClientPtr = std::shared_ptr<StorageUnitClient>;

  using Client    = StorageUnitClient::Client;
  using ClientPtr = StorageUnitClient::ClientPtr;
  using Address   = StorageUnitClient::Address;

  explicit LaneRemoteControl(StorageUnitClientPtr storage_unit)
    : storage_unit_(std::move(storage_unit))
  {}

  LaneRemoteControl(LaneRemoteControl const &other) = default;
  LaneRemoteControl(LaneRemoteControl &&other)      = default;
  LaneRemoteControl &operator=(LaneRemoteControl const &other) = default;
  LaneRemoteControl &operator=(LaneRemoteControl &&other) = default;
  ~LaneRemoteControl() override                           = default;

  void Shutdown(LaneIndex lane) override
  {
    auto    ptr = GetClient();
    Address address;
    if (ptr && GetAddressForLane(lane, address))
    {
      try
      {
        auto p =
            ptr->CallSpecificAddress(address, RPC_CONTROLLER, LaneControllerProtocol::SHUTDOWN);
        FETCH_LOG_PROMISE();
        p->Wait();
      }
      catch (...)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Remote lane shutdown call failed.");
        throw;
      }
    }
  }

  uint32_t GetLaneNumber(LaneIndex lane) override
  {
    auto    ptr = GetClient();
    Address address;
    if (ptr && GetAddressForLane(lane, address))
    {
      try
      {
        auto p =
            ptr->CallSpecificAddress(address, RPC_IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
        return p->As<uint32_t>();
      }
      catch (...)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to execute remote GET_LANE_NUMBER");
        throw;
      }
    }
    TODO_FAIL("client connection has died, @GetLaneNumber");

    return 0;
  }

  int IncomingPeers(LaneIndex lane) override
  {
    auto    ptr = GetClient();
    Address address;
    if (ptr && GetAddressForLane(lane, address))
    {
      try
      {
        auto p = ptr->CallSpecificAddress(address, RPC_CONTROLLER,
                                          LaneControllerProtocol::INCOMING_PEERS);
        return p->As<int>();
      }
      catch (...)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to execute remote INCOMING_PEERS");
        throw;
      }
    }

    TODO_FAIL("client connection has died, @IncomingPeers");

    return 0;
  }

  int OutgoingPeers(LaneIndex lane) override
  {
    auto    ptr = GetClient();
    Address address;
    if (ptr && GetAddressForLane(lane, address))
    {
      try
      {
        auto p = ptr->CallSpecificAddress(address, RPC_CONTROLLER,
                                          LaneControllerProtocol::OUTGOING_PEERS);
        return p->As<int>();
      }
      catch (...)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to exxecute remote OUTGOING_PEERS");
        throw;
      }
    }

    TODO_FAIL("client connection has died, @OutgoingPeers");

    return 0;
  }

  virtual void UseThesePeers(LaneIndex lane, const std::unordered_set<Uri> &uris) override
  {
    auto    ptr = GetClient();
    Address address;
    if (ptr && GetAddressForLane(lane, address))
    {
      try
      {
        auto p = ptr->CallSpecificAddress(address, RPC_CONTROLLER,
                                          LaneControllerProtocol::USE_THESE_PEERS, uris);
        return;
      }
      catch (...)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to exxecute remote USE_THESE_PEERS");
        throw;
      }
    }
    TODO_FAIL("client connection has died, @UseThesePeers");
  }

  bool IsAlive(LaneIndex lane) override
  {
    Address address;
    return GetAddressForLane(lane, address);
  }

private:
  ClientPtr GetClient() const
  {
    return storage_unit_->GetClient();
  }
  bool GetAddressForLane(LaneIndex lane, Address &address) const
  {
    try
    {
      storage_unit_->GetAddressForLane(lane, address);
    }
    catch (...)
    {
      return false;
    }
    return true;
  }

  StorageUnitClientPtr storage_unit_;
  mutable Mutex        mutex_{__LINE__, __FILE__};
};

}  // namespace ledger
}  // namespace fetch
