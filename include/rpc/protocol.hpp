#ifndef RPC_PROTOCOL_HPP
#define RPC_PROTOCOL_HPP

#include "assert.hpp"
#include "rpc/callable_class_member.hpp"
#include "serializer/referenced_byte_array.hpp"

#include <map>
#include "rpc/error_codes.hpp"

namespace fetch {
namespace rpc {

class Protocol {
 public:
  typedef AbstractCallable callable_type;
  typedef byte_array::ReferencedByteArray byte_array_type;

  callable_type& operator[](function_handler_type const& n) {
    if(members_[n] == nullptr)
      throw serializers::SerializableException(
          error::MEMBER_NOT_FOUND,
          byte_array_type("Could not find member "));
    return *members_[n];
  }

  void Expose(function_handler_type const& n, callable_type* fnc) {
    if(members_[n] != nullptr)
      throw serializers::SerializableException(
          error::MEMBER_EXISTS,
          byte_array_type("Member already exists: "));

    members_[n] = fnc;
  }

  void Publish(uint16_t const &stream) {

  }
  
 private:
  callable_type* members_[256] = {nullptr};
};
};
};

#endif
