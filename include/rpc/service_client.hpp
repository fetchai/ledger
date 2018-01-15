#ifndef RPC_SERVICE_CLIENT_HPP
#define RPC_SERVICE_CLIENT_HPP

#include "rpc/callable_class_member.hpp"
#include "rpc/message_types.hpp"
#include "rpc/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "rpc/error_codes.hpp"
#include "rpc/promise.hpp"

#include "assert.hpp"
#include "network/network_client.hpp"

#include <map>

namespace fetch {
namespace rpc {

class ServiceClient : public network::NetworkClient {
 public:
  typedef byte_array::ReferencedByteArray byte_array_type;

  ServiceClient(byte_array_type const& host, uint16_t const& port)
      : NetworkClient(host, port) {}

  template <typename... arguments>
  Promise Call(byte_array::ReferencedByteArray protocol,
               byte_array::ReferencedByteArray function, arguments... args) {
    Promise prom;
    serializer_type params;
    params << RPC_FUNCTION_CALL << prom.id();

    promises_[prom.id()] = prom.reference();

    PackCall(params, protocol, function, args...);
    Send(params.data());

    return prom;
  }

  void PushMessage(network::message_type const& msg) override {
    serializer_type params(msg);

    rpc_classification_type type;
    params >> type;

    if (type == RPC_RESULT) {
      Promise::promise_counter_type id;
      params >> id;
      auto it = promises_.find(id);
      if (it == promises_.end()) {
        throw serializers::SerializableException(
            error::PROMISE_NOT_FOUND,
            byte_array_type("Could not find promise"));
      }

      auto ret = msg.SubArray(params.Tell(), msg.size() - params.Tell());
      it->second->Fulfill(ret.Copy());

      promises_.erase(it);
    } else if (type == RPC_ERROR) {
      Promise::promise_counter_type id;
      params >> id;

      serializers::SerializableException e;
      params >> e;

      auto it = promises_.find(id);
      if (it == promises_.end()) {
        throw serializers::SerializableException(
            error::PROMISE_NOT_FOUND,
            byte_array_type("Could not find promise"));
      }

      it->second->Fail(e);

      promises_.erase(it);
    } else {
      throw serializers::SerializableException(
          error::UNKNOWN_MESSAGE, byte_array_type("Unknown message"));
    }
  }

 private:
  std::map<Promise::promise_counter_type, Promise::shared_promise_type>
      promises_;
};
};
};

#endif
