#ifndef RPC_SERVICE_SERVER_HPP
#define RPC_SERVICE_SERVER_HPP

#include "rpc/callable_class_member.hpp"
#include "rpc/message_types.hpp"
#include "rpc/protocol.hpp"
#include "serializer/referenced_byte_array.hpp"
#include "serializer/serializable_exception.hpp"

#include "rpc/error_codes.hpp"
#include "rpc/promise.hpp"

#include "assert.hpp"
#include "network/network_server.hpp"

#include <map>

namespace fetch {
namespace rpc {

class ServiceServer : public network::NetworkServer {
 public:
  typedef byte_array::ReferencedByteArray byte_array_type;

  ServiceServer(uint16_t port) : NetworkServer(port) {}

  void Start() override {
    NetworkServer::Start();

    // TODO: implement threading
    /*
    if( worker_thread_ == nullptr) {
      worker_thread_ = new std::thread([this]() {

        });
    }
    */
  }

  void Stop() override {
    NetworkServer::Stop();
    /*
    if( worker_thread_ != nullptr) {
      worker_thread_->join();
      delete worker_thread_;
    }
    */
  }

  void PushRequest(handle_type client,
                   network::message_type const& msg) override {
    // FIXME: Change to threaded model to avoid blocking
    serializer_type params(msg);

    rpc_classification_type type;
    params >> type;

    if (type == RPC_FUNCTION_CALL) {
      serializer_type result;
      Promise::promise_counter_type id;
      params >> id;
      try {
        result << RPC_RESULT << id;
        Call(result, params);
      } catch (serializers::SerializableException const& e) {
        result = serializer_type();
        result << RPC_ERROR << id << e;
      }

      Respond(client, result.data());
    } else {
      TODO_FAIL("call type not implemented yet");
    }
  }

  void Add(byte_array_type const& name, Protocol* fnc) {
    if (members_.find(name) != members_.end()) {
      throw serializers::SerializableException(
          error::PROTOCOL_EXISTS,
          byte_array_type("Member already exists: ") + name);
    }

    members_[name] = fnc;
  }

 private:
  void Call(serializer_type& result, serializer_type params) {
    byte_array_type protocol, function;
    params >> protocol >> function;

    auto it = members_.find(protocol);
    if (it == members_.end()) {
      throw serializers::SerializableException(
          error::PROTOCOL_NOT_FOUND,
          byte_array_type("Could not find protocol: ") + protocol);
    }

    auto& mod = *it->second;
    return mod[function](result, params);
  }

  std::map<byte_array_type, Protocol*> members_;
};
};
};

#endif
