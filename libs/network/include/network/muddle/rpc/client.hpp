#pragma once

#include "network/service/types.hpp"
#include "network/service/promise.hpp"
#include "network/service/client_interface.hpp"
#include "network/muddle/muddle_endpoint.hpp"
#include "network/details/thread_pool.hpp"

#include <memory>

namespace fetch {
namespace muddle {
namespace rpc {

class Client : public service::ServiceClientInterface
{
public:
  using Address = MuddleEndpoint::Address;
  using ProtocolId = service::protocol_handler_type;
  using FunctionId = service::function_handler_type;
  using Serializer = service::serializer_type;
  using Promise = service::Promise;
  using ThreadPool = network::ThreadPool;
  using Handler = std::function<void(Promise)>;
  using SharedHandler = std::shared_ptr<Handler>;
  using WeakHandler = std::weak_ptr<Handler>;

  static constexpr char const *LOGGING_NAME = "RpcClient";

  Client(MuddleEndpoint &endpoint, Address const &address, uint16_t service, uint16_t channel)
    : endpoint_(endpoint)
    , address_(address)
    , service_(service)
    , channel_(channel)
  {
    handler_ = std::make_shared<Handler>([this](Promise promise) {
      ProcessServerMessage(promise->value());
    });

    thread_pool_->Start();
  }

  ~Client() override
  {
    // clear that handler
    handler_.reset();

    thread_pool_->Stop();
  }

  void SetAddress(Address const &address)
  {
    address_ = address;
  }

protected:

  bool DeliverRequest(network::message_type const &data) override
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Please send this packet to the server");

    // signal to the networking that an exchange is requested
    auto promise = endpoint_.Exchange(address_, service_, channel_, data);

    // establish the correct course of action when
    WeakHandler handler = handler_;
    promise.WithHandlers()
      .Then([handler, promise]() {
        FETCH_LOG_INFO(LOGGING_NAME, "Got the response to our question...");
        auto callback = handler.lock();
        if (callback)
        {
          (*callback)(promise.GetInnerPromise());
        }
      })
      .Catch([]() {

        // TODO(EJF): This is actually a bug since the RPC promise implementation doesn't have a callback process
        FETCH_LOG_INFO(LOGGING_NAME, "Exchange promise failed");
      });

    // TODO(EJF): Chained promises would remove the requirement for this
    thread_pool_->Post([promise]() { promise.Wait(); });

    return true; //?
  }

private:

  MuddleEndpoint &endpoint_;
  Address address_;
  uint16_t const service_;
  uint16_t const channel_;
  ThreadPool thread_pool_ = network::MakeThreadPool(1);
  SharedHandler handler_;
};

} // namespace rpc
} // namespace muddle
} // namespace fetch
