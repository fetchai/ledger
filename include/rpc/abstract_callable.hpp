#ifndef RPC_ABSTRACT_CALLABLE_HPP
#define RPC_ABSTRACT_CALLABLE_HPP
#include "rpc/types.hpp"

namespace fetch {
namespace rpc {


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
              protocol_handler_type const& protocol,
              function_handler_type const& function, arguments... args) {
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
