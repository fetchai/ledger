#pragma once
#include "ledger/storage_unit/lane_controller_protocol.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "network/service/client.hpp"

#include <unordered_map>
namespace fetch {
namespace ledger {

class LaneRemoteControl
{
public:
  using service_type        = service::ServiceClient;
  using shared_service_type = std::shared_ptr<service_type>;
  using weak_service_type   = std::weak_ptr<service_type>;
  using lane_index_type     = uint32_t;

  enum
  {
    CONTROLLER_PROTOCOL_ID = LaneService::CONTROLLER,
    IDENTITY_PROTOCOL_ID   = LaneService::IDENTITY
  };

  LaneRemoteControl() {}
  LaneRemoteControl(LaneRemoteControl const &other) = default;
  LaneRemoteControl(LaneRemoteControl &&other)      = default;
  LaneRemoteControl &operator=(LaneRemoteControl const &other) = default;
  LaneRemoteControl &operator=(LaneRemoteControl &&other) = default;

  ~LaneRemoteControl() = default;

  void AddClient(lane_index_type const &lane, weak_service_type const &client)
  {
    clients_[lane] = client;
  }

  void Connect(lane_index_type const &lane, byte_array::ByteArray const &host, uint16_t const &port)
  {
    if (clients_.find(lane) == clients_.end())
    {
      TODO_FAIL("Client not found");
    }

    auto ptr = clients_[lane].lock();
    if (ptr)
    {
      fetch::logger.Info("Remote lane call to: ", host, ":", port);
      auto p = ptr->Call(CONTROLLER_PROTOCOL_ID, LaneControllerProtocol::CONNECT, host, port);

      FETCH_LOG_PROMISE();
      p.Wait();
    }
  }

  void TryConnect(lane_index_type const &lane, p2p::EntryPoint const &ep)
  {
    auto ptr = clients_[lane].lock();
    if (ptr)
    {
      auto p = ptr->Call(CONTROLLER_PROTOCOL_ID, LaneControllerProtocol::TRY_CONNECT, ep);

      FETCH_LOG_PROMISE();
      p.Wait();
    }
  }

  void Shutdown(lane_index_type const &lane)
  {
    if (clients_.find(lane) == clients_.end())
    {
      TODO_FAIL("Client not found");
    }

    auto ptr = clients_[lane].lock();
    if (ptr)
    {
      auto p = ptr->Call(CONTROLLER_PROTOCOL_ID, LaneControllerProtocol::SHUTDOWN);

      FETCH_LOG_PROMISE();
      p.Wait();
    }
  }

  uint32_t GetLaneNumber(lane_index_type const &lane)
  {
    if (clients_.find(lane) == clients_.end())
    {
      TODO_FAIL("Client not found");
    }

    auto ptr = clients_[lane].lock();
    if (ptr)
    {
      auto p = ptr->Call(IDENTITY_PROTOCOL_ID, LaneIdentityProtocol::GET_LANE_NUMBER);
      return p.As<uint32_t>();
    }

    TODO_FAIL("client connection has died");

    return 0;
  }

  int IncomingPeers(lane_index_type const &lane)
  {
    if (clients_.find(lane) == clients_.end())
    {
      TODO_FAIL("Client not found");
    }

    auto ptr = clients_[lane].lock();
    if (ptr)
    {
      auto p = ptr->Call(CONTROLLER_PROTOCOL_ID, LaneControllerProtocol::INCOMING_PEERS);
      return p.As<int>();
    }

    TODO_FAIL("client connection has died");

    return 0;
  }

  int OutgoingPeers(lane_index_type const &lane)
  {
    if (clients_.find(lane) == clients_.end())
    {
      TODO_FAIL("Client not found");
    }

    auto ptr = clients_[lane].lock();
    if (ptr)
    {
      auto p = ptr->Call(CONTROLLER_PROTOCOL_ID, LaneControllerProtocol::OUTGOING_PEERS);
      return p.As<int>();
    }

    TODO_FAIL("client connection has died");

    return 0;
  }

  bool IsAlive(lane_index_type const &lane)
  {
    if (clients_.find(lane) == clients_.end())
    {
      TODO_FAIL("Client not found");
    }

    auto ptr = clients_[lane].lock();
    return bool(ptr);
  }

private:
  std::unordered_map<lane_index_type, weak_service_type> clients_;
};

}  // namespace ledger
}  // namespace fetch
