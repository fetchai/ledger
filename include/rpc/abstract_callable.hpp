#ifndef RPC_ABSTRACT_CALLABLE_HPP
#define RPC_ABSTRACT_CALLABLE_HPP
#include "serializer/referenced_byte_array.hpp"
#include "serializer/stl_types.hpp"
#include "serializer/typed_byte_array_buffer.hpp"

namespace fetch {
namespace rpc {
typedef serializers::TypedByte_ArrayBuffer serializer_type;

namespace details {

struct Packer {
  template <typename T, typename... arguments>
  static serializer_type &SerializeArguments(serializer_type &serializer,
                                             T &next, arguments... args) {
    serializer << next;
    return SerializeArguments(serializer, args...);
  }

  template <typename T>
  static serializer_type &SerializeArguments(serializer_type &serializer,
                                             T &next) {
    serializer << next;
    serializer.Seek(0);
    return serializer;
  }
};
};

template <typename... arguments>
void PackCall(serializer_type &serializer,
              byte_array::ReferencedByteArray protocol,
              byte_array::ReferencedByteArray function, arguments... args) {
  serializer << protocol;
  serializer << function;
  details::Packer::SerializeArguments(serializer, args...);
}

class AbstractCallable {
 public:
  virtual ~AbstractCallable(){};
  virtual void operator()(serializer_type &result, serializer_type &params) = 0;
};
};
};

#endif
