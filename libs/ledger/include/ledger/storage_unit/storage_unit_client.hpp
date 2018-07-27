#pragma once
#include "core/logger.hpp"
#include "ledger/storage_unit/lane_connectivity_details.hpp"
#include "ledger/storage_unit/lane_identity.hpp"
#include "ledger/storage_unit/lane_identity_protocol.hpp"
#include "ledger/storage_unit/lane_service.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "network/management/connection_register.hpp"
#include "network/service/client.hpp"

#include "ledger/chain/transaction.hpp"
#include "storage/document_store_protocol.hpp"
#include "storage/object_store_protocol.hpp"

#include <chrono>
#include <thread>

namespace fetch {
namespace ledger {

class StorageUnitClient : public StorageUnitInterface
{
public:
  struct ClientDetails
  {
    crypto::Identity      identity;
    std::atomic<uint32_t> lane;
  };

  using service_client_type = service::ServiceClient;
  using shared_service_client_type = std::shared_ptr<service_client_type>;
  using client_register_type   = fetch::network::ConnectionRegister<ClientDetails>;
  using connection_handle_type = client_register_type::connection_handle_type;
  using network_manager_type   = fetch::network::NetworkManager;
  using lane_type              = LaneIdentity::lane_type;

  // TODO(EJF):  is move?
  explicit StorageUnitClient(network_manager_type tm) : network_manager_(tm)
  {
    id_ = "my-fetch-id";
    // libs/ledger/include/ledger/chain/helper_functions.hpp
  }

  void SetNumberOfLanes(lane_type const &count)
  {
    lanes_.resize(count);
    SetLaneLog2(uint32_t(lanes_.size()));
    assert(count == (1 << log2_lanes_));
  }

  template <typename T>
  crypto::Identity AddLaneConnection(byte_array::ByteArray const &host, uint16_t const &port)
  {
    shared_service_client_type client =
        register_.template CreateServiceClient<T>(network_manager_, host, port);

    // Waiting for connection to be open
    bool connection_timeout = false;
    for (std::size_t n = 0; n < 10; ++n)
    {

      // ensure the connection is live
      if (client->is_alive())
      {

        // make the client call
        auto p = client->Call(LaneService::IDENTITY, LaneIdentityProtocol::PING);
        if (p.Wait(100, false))
        {
          if (p.As<LaneIdentity::ping_type>() != LaneIdentity::PING_MAGIC)
          {
            connection_timeout = true;
          }
          break;
        }
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
      }
    }

    if (connection_timeout)
    {
      logger.Warn("Connection timed out - closing");
      client->Close();
      client.reset();
      return crypto::InvalidIdentity();
    }

    // Exchaning info
    auto p1 = client->Call(LaneService::IDENTITY, LaneIdentityProtocol::GET_LANE_NUMBER);
    auto p2 = client->Call(LaneService::IDENTITY, LaneIdentityProtocol::GET_TOTAL_LANES);
    auto p3 = client->Call(LaneService::IDENTITY, LaneIdentityProtocol::GET_IDENTITY);
    if ((!p1.Wait(1000)) || (!p2.Wait(1000)) || (!p3.Wait(1000)))
    {
      fetch::logger.Warn("Client timeout when trying to get identity details.");
      client->Close();
      client.reset();
      return crypto::InvalidIdentity();
    }

    lane_type lane        = p1.As<lane_type>();
    lane_type total_lanes = p2.As<lane_type>();
    if (total_lanes > lanes_.size())
    {
      lanes_.resize(total_lanes);
      SetLaneLog2(uint32_t(lanes_.size()));
      assert(lanes_.size() == (1 << log2_lanes_));
    }

    crypto::Identity lane_identity;
    p3.As(lane_identity);
    // TODO: Verify expected identity

    assert(lane < lanes_.size());
    fetch::logger.Debug("Adding lane ", lane);

    lanes_[lane] = client;

    {
      auto details      = register_.GetDetails(client->handle());
      details->identity = lane_identity;
    }

    return lane_identity;
  }

  void TryConnect(p2p::EntryPoint const &ep)
  {
    if (ep.lane_id < lanes_.size())
    {
      lanes_[ep.lane_id]->Call(LaneService::CONTROLLER, LaneControllerProtocol::TRY_CONNECT, ep);
    }
  }

  void AddTransaction(chain::VerifiedTransaction const &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto        res     = fetch::storage::ResourceID(tx.digest());
    std::size_t lane    = res.lane(log2_lanes_);
    auto        promise = lanes_[lane]->Call(LaneService::TX_STORE, protocol::SET, res, tx);
    promise.Wait();
  }

  bool GetTransaction(byte_array::ConstByteArray const &digest, chain::Transaction &tx) override
  {
    using protocol = fetch::storage::ObjectStoreProtocol<chain::Transaction>;

    auto        res     = fetch::storage::ResourceID(digest);
    std::size_t lane    = res.lane(log2_lanes_);
    auto        promise = lanes_[lane]->Call(LaneService::TX_STORE, protocol::GET, res);
    tx                  = promise.As<chain::VerifiedTransaction>();

    return true;
  }

  document_type GetOrCreate(byte_array::ConstByteArray const &key) override
  {
    auto        res  = fetch::storage::ResourceID(key);
    std::size_t lane = res.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::GET_OR_CREATE, res);

    return promise.As<storage::Document>();
  }

  document_type Get(byte_array::ConstByteArray const &key) override
  {
    auto        res  = fetch::storage::ResourceID(key);
    std::size_t lane = res.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(LaneService::STATE,
                                      fetch::storage::RevertibleDocumentStoreProtocol::GET, res);

    return promise.As<storage::Document>();
  }

  bool Lock(byte_array::ConstByteArray const &key) override
  {
    auto        res     = fetch::storage::ResourceID(key);
    std::size_t lane    = res.lane(log2_lanes_);
    auto        promise = lanes_[lane]->Call(LaneService::STATE,
                                      fetch::storage::RevertibleDocumentStoreProtocol::LOCK, res);

    return promise.As<bool>();
  }

  bool Unlock(byte_array::ConstByteArray const &key) override
  {

    auto        res     = fetch::storage::ResourceID(key);
    std::size_t lane    = res.lane(log2_lanes_);
    auto        promise = lanes_[lane]->Call(LaneService::STATE,
                                      fetch::storage::RevertibleDocumentStoreProtocol::UNLOCK, res);

    return promise.As<bool>();
  }

  void Set(byte_array::ConstByteArray const &key, byte_array::ConstByteArray const &value) override
  {
    auto        res  = fetch::storage::ResourceID(key);
    std::size_t lane = res.lane(log2_lanes_);

    auto promise = lanes_[lane]->Call(
        LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::SET, res, value);
    promise.Wait(2000);
  }

  void Commit(bookmark_type const &bookmark) override
  {
    std::vector<service::Promise> promises;
    for (std::size_t i = 0; i < lanes_.size(); ++i)
    {
      auto promise = lanes_[i]->Call(
          LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::COMMIT, bookmark);
      promises.push_back(promise);
    }

    for (auto &p : promises)
    {
      p.Wait();
    }
  }

  void Revert(bookmark_type const &bookmark) override
  {
    std::vector<service::Promise> promises;
    for (std::size_t i = 0; i < lanes_.size(); ++i)
    {
      auto promise = lanes_[i]->Call(
          LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::REVERT, bookmark);
      promises.push_back(promise);
    }

    for (auto &p : promises)
    {
      p.Wait();
    }
  }

  byte_array::ConstByteArray Hash() override
  {
    // TODO
    return lanes_[0]
        ->Call(LaneService::STATE, fetch::storage::RevertibleDocumentStoreProtocol::HASH)
        .As<byte_array::ByteArray>();
  }

  void SetID(byte_array::ByteArray const &id) { id_ = id; }

  byte_array::ByteArray const &id() { return id_; }

  std::size_t lanes() const { return lanes_.size(); }

  bool is_alive() const
  {
    bool alive = true;
    for (auto &lane : lanes_)
    {
      if (!lane->is_alive())
      {
        alive = false;
        break;
      }
    }
    return alive;
  }

private:
  network_manager_type network_manager_;
  uint32_t             log2_lanes_ = 0;

  void SetLaneLog2(lane_type const &count)
  {
    log2_lanes_ = uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(count)) + 1));
  }

  client_register_type                    register_;
  byte_array::ByteArray                   id_;
  std::vector<shared_service_client_type> lanes_;
};

}  // namespace ledger
}  // namespace fetch
