#ifndef SERVICE_SERVER_INTERFACE_HPP
#define SERVICE_SERVER_INTERFACE_HPP
#include "core/byte_array/byte_array.hpp"
#include "network/message.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/message_types.hpp"
#include "network/service/promise.hpp"
#include "network/service/protocol.hpp"
#include "network/service/types.hpp"

namespace fetch {
namespace service {

class ServiceServerInterface {
 public:
  typedef network::AbstractConnection::connection_handle_type connection_handle_type;
  typedef byte_array::ConstByteArray byte_array_type;

  virtual ~ServiceServerInterface() {}

  void Add(protocol_handler_type const& name,
           Protocol* protocol)  // TODO: Rename to AddProtocol
  {
    LOG_STACK_TRACE_POINT;

    // TODO: (`HUT`) : better reporting of errors
    if (members_[name] != nullptr) {
      throw serializers::SerializableException(
          error::PROTOCOL_EXISTS, byte_array_type("Protocol already exists. "));
    }

    members_[name] = protocol;

    for (auto& feed : protocol->feeds()) {
      feed->AttachToService(this);
    }
  }

 protected:
  virtual bool DeliverResponse(connection_handle_type, network::message_type const&) = 0;

  bool PushProtocolRequest(connection_handle_type client,
                           network::message_type const& msg) {
    LOG_STACK_TRACE_POINT;
    bool ret = false;
    serializer_type params(msg);

    service_classification_type type;
    params >> type;

    if (type == SERVICE_FUNCTION_CALL)  // TODO: change to switch
    {
      ret = true;
      serializer_type result;
      Promise::promise_counter_type id;

      try {
        params >> id;
        result << SERVICE_RESULT << id;

        ExecuteCall(result, client, params);
      } catch (serializers::SerializableException const& e) {
        fetch::logger.Error("Serialization error: ", e.what());
        result = serializer_type();
        result << SERVICE_ERROR << id << e;
      }

      fetch::logger.Debug("Service Server responding to call from ", client);
      DeliverResponse(client, result.data());
    } else if (type == SERVICE_SUBSCRIBE) {
      ret = true;
      protocol_handler_type protocol;
      feed_handler_type feed;
      subscription_handler_type subid;

      try {
        params >> protocol >> feed >> subid;
        auto& mod = *members_[protocol];

        mod.Subscribe(client, feed, subid);

      } catch (serializers::SerializableException const& e) {
        fetch::logger.Error("Serialization error: ", e.what());
        // FIX Serialization of errors such that this also works

        //          serializer_type result;
        //          result = serializer_type();
        //          result << SERVICE_ERROR << id << e;
        //          Send(client, result.data());

        throw e;
      }

    } else if (type == SERVICE_UNSUBSCRIBE) {
      ret = true;
      protocol_handler_type protocol;
      feed_handler_type feed;
      subscription_handler_type subid;

      try {
        params >> protocol >> feed >> subid;
        auto& mod = *members_[protocol];

        mod.Unsubscribe(client, feed, subid);

      } catch (serializers::SerializableException const& e) {
        fetch::logger.Error("Serialization error: ", e.what());
        // FIX Serialization of errors such that this also works

        //          serializer_type result;
        //          result = serializer_type();
        //          result << SERVICE_ERROR << id << e;
        //          Send(client, result.data());

        throw e;
      }
    }

    return ret;
  }

 private:
  void ExecuteCall(serializer_type& result, connection_handle_type const& client,
                   serializer_type params) {
    //    LOG_STACK_TRACE_POINT;

    protocol_handler_type protocol;
    function_handler_type function;
    params >> protocol >> function;
    fetch::logger.Debug("Service Server processing call ", protocol, ":",
                        function, " from ", client);

    if (members_[protocol] == nullptr) {
      throw serializers::SerializableException(
          error::PROTOCOL_NOT_FOUND,
          byte_array_type("Could not find protocol: "));
    }

    auto& mod = *members_[protocol];

    mod.ApplyMiddleware(client, params.data());    
    
    auto& fnc = mod[function];

    // If we need to add client id to function arguments
    if (fnc.meta_data() & Callable::CLIENT_ID_ARG) {
      CallableArgumentList extra_args;
      extra_args.PushArgument(&client);
      return fnc(result, extra_args, params);
    }

    return fnc(result, params);
  }

  Protocol* members_[256] = {nullptr};  // TODO: Not thread-safe
  friend class FeedSubscriptionManager;
};
}
}
#endif
