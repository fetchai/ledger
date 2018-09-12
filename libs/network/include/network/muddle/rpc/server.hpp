#pragma once

#include "core/mutex.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/service/server_interface.hpp"
#include "network/tcp/tcp_server.hpp"

#include <unordered_map>
#include <tuple> // emulation layer
#include <array>

namespace fetch {
namespace muddle {
namespace rpc {

class Server : public service::ServiceServerInterface
{
public:
  using ConnectionHandle = service::ServiceServerInterface::connection_handle_type;
  using ProtocolId = service::protocol_handler_type;
  using Protocol = service::Protocol;
  using Address = MuddleEndpoint::Address;
  using SubscriptionPtr = MuddleEndpoint::SubscriptionPtr ;
  using SubscriptionMap = std::unordered_map<ProtocolId, SubscriptionPtr>;
  using Mutex = mutex::Mutex;

  static constexpr char const *LOGGING_NAME = "RpcServer";

  explicit Server(MuddleEndpoint &endpoint, uint16_t service, uint16_t channel)
    : endpoint_(endpoint)
    //, service_(service)
    //, channel_(channel)
    , subscription_(endpoint_.Subscribe(service, channel))
  {
    if (subscription_)
    {
      // register the subscription with our handler
      subscription_->SetMessageHandler(
        [this](Address const &from, uint16_t service, uint16_t channel, uint16_t counter, Packet::Payload const &payload)
        {
          OnMessage(from, service, channel, counter, payload);
        }
      );
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to configure data subscription");
    }
  }

protected:

  bool DeliverResponse(connection_handle_type handle_type,
                       network::message_type const &message_type) override
  {
    Address target;
    uint16_t service = 0;
    uint16_t channel = 0;
    uint16_t counter = 0;
    bool lookup_success = false;

    // lookup the metadata
    {
      FETCH_LOCK(metadata_lock_);
      auto it = metadata_.find(handle_type);
      if (it != metadata_.end())
      {
        std::tie(target, service, channel, counter) = metadata_[handle_type];
        metadata_.erase(it);
        lookup_success = true;
      }
    }

    if (lookup_success)
    {
      // informat the world
      FETCH_LOG_INFO(LOGGING_NAME, "Sending message to: ", byte_array::ToBase64(target), " on: ",
                     service, ':', channel, ':', counter);

      // send the message back to the server
      endpoint_.Send(target, service, channel, counter, message_type);
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to determine which person to callback from");
    }

    return true; /// ?
  }

private:

  void OnMessage(Address const &from, uint16_t service, uint16_t channel, uint16_t counter, Packet::Payload const &payload)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Recv message from: ", byte_array::ToBase64(from), " on: ", service, ':', channel, ':', counter);

    // insert data into the metadata
    uint64_t index = 0;
    {
      FETCH_LOCK(metadata_lock_);
      index = metadata_index_++;
      metadata_[index] = {from, service, channel, counter};
    }

    // dispatch down to the core RPC level
    PushProtocolRequest(index, payload);
  }

  MuddleEndpoint &endpoint_;
  //uint16_t const  service_;
  //uint16_t const  channel_;
  SubscriptionPtr subscription_;

  // begin annoying emulation layer
  using Metadata = std::tuple<Address, uint16_t, uint16_t, uint16_t>;
  using MetadataMap = std::unordered_map<uint64_t, Metadata>;

  Mutex           metadata_lock_{__LINE__, __FILE__};
  uint64_t        metadata_index_ = 0;
  MetadataMap     metadata_;
};

} // namespace rpc
} // namespace muddle
} // namespace fetch
