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

  callable_type& operator[](byte_array_type const& str) {
    auto it = members_.find(str);
    if (it == members_.end()) {
      throw serializers::SerializableException(
          error::MEMBER_NOT_FOUND,
          byte_array_type("Could not find member ") + str);
    }
    return *it->second;
  }

  void Expose(byte_array_type const& name, callable_type* fnc) {
    if (members_.find(name) != members_.end()) {
      throw serializers::SerializableException(
          error::MEMBER_EXISTS,
          byte_array_type("Member already exists: ") + name);
    }

    members_[name] = fnc;
  }

 private:
  std::map<byte_array_type, callable_type*> members_;
};
};
};

#endif
