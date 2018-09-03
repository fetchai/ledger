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
#include "network/p2pservice/p2p_lane_management.hpp"
#include "ledger/storage_unit/lane_controller_protocol.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "network/service/client.hpp"

#include <unordered_map>
namespace fetch {
namespace ledger {

class LaneRemoteControl : public p2p::LaneManagement
{
public:
  static constexpr char const *LOGGING_NAME = "LaneRemoteControl";

  using Mutex = mutex::Mutex;
  using Promise = service::Promise;

  explicit LaneRemoteControl(std::size_t num_lanes)
    : clients_{num_lanes}
  {}

  LaneRemoteControl(LaneRemoteControl const &other) = default;
  LaneRemoteControl(LaneRemoteControl &&other)      = default;
  LaneRemoteControl &operator=(LaneRemoteControl const &other) = default;
  LaneRemoteControl &operator=(LaneRemoteControl &&other) = default;
  ~LaneRemoteControl() override = default;

  void AddClient(LaneIndex lane, WeakService const &client)
  {
    FETCH_LOCK(mutex_);
    clients_.at(lane) = client;
  }

  void Connect(LaneIndex lane, ConstByteArray const &host, uint16_t port) override
  {
    auto ptr = LookupLane(lane);
    if (ptr)
    {
      FETCH_LOG_INFO(LOGGING_NAME,"Remote lane call to: ", host, ":", port);
      auto p = ptr->Call(RPC_CONTROLLER, LaneControllerProtocol::CONNECT, host, port);

      FETCH_LOG_PROMISE();
      p->Wait();
    }
  }

  void TryConnect(LaneIndex lane, p2p::EntryPoint const &ep)
  {
    auto ptr = LookupLane(lane);
    if (ptr)
    {
      auto p = ptr->Call(RPC_CONTROLLER, LaneControllerProtocol::TRY_CONNECT, ep);

      FETCH_LOG_PROMISE();
      p->Wait();
    }
  }

  void Shutdown(LaneIndex lane) override
  {
    auto ptr = LookupLane(lane);
    if (ptr)
    {
      auto p = ptr->Call(RPC_CONTROLLER, LaneControllerProtocol::SHUTDOWN);

      FETCH_LOG_PROMISE();
      p->Wait();
    }
  }

  uint32_t GetLaneNumber(LaneIndex lane) override
  {
    auto ptr = LookupLane(lane);
    if (ptr)
    {
      auto p = ptr->Call(RPC_IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
      return p->As<uint32_t>();
    }

    TODO_FAIL("client connection has died");

    return 0;
  }

  int IncomingPeers(LaneIndex lane) override
  {
    auto ptr = LookupLane(lane);
    if (ptr)
    {
      auto p = ptr->Call(RPC_CONTROLLER, LaneControllerProtocol::INCOMING_PEERS);
      return p->As<int>();
    }

    TODO_FAIL("client connection has died");

    return 0;
  }

  int OutgoingPeers(LaneIndex lane) override
  {
    auto ptr = LookupLane(lane);

    if (ptr)
    {
      auto p = ptr->Call(RPC_CONTROLLER, LaneControllerProtocol::OUTGOING_PEERS);
      return p->As<int>();
    }

    TODO_FAIL("client connection has died");

    return 0;
  }

  bool IsAlive(LaneIndex lane) override
  {
    return static_cast<bool>(LookupLane(lane));
  }

private:


  SharedService LookupLane(LaneIndex lane) const
  {
    FETCH_LOCK(mutex_);

#ifdef NDEBUG
    WeakService service = clients_[lane];
#else
    WeakService service = clients_.at(lane);
#endif

    return service.lock();
  }

  mutable Mutex mutex_{__LINE__, __FILE__};
  std::vector<WeakService> clients_;
};

}  // namespace ledger
}  // namespace fetch
