#ifndef RPC_ABSTRACT_CALLABLE_HPP
#define RPC_ABSTRACT_CALLABLE_HPP
#include "rpc/types.hpp"

namespace fetch {
namespace rpc {


namespace details {

template <typename T, typename... arguments>  
struct Packer {
  static void SerializeArguments(serializer_type &serializer,
                                             T &next, arguments... args) {
    serializer << next;
    Packer<arguments... >::SerializeArguments(serializer, args...);
  }
};
  
template < typename T >  
struct Packer<T> { 
  static void SerializeArguments(serializer_type &serializer,
                                 T &next) {
    serializer << next;
    serializer.Seek(0);
  }
};

  
};


template <typename... arguments>
void PackCall(serializer_type &serializer,
              protocol_handler_type const& protocol,
              function_handler_type const& function, arguments... args) {
  serializer << protocol;
  serializer << function;
  details::Packer<arguments... >::SerializeArguments(serializer, args...);
}

void PackCall(serializer_type &serializer,
              protocol_handler_type const& protocol,
              function_handler_type const& function) {
  serializer << protocol;
  serializer << function;
  serializer.Seek(0);
}


template <typename... arguments>
void PackArgs(serializer_type &serializer, arguments... args) {
  details::Packer<arguments... >::SerializeArguments(serializer, args...);
}

void PackArgs(serializer_type &serializer) {
  serializer.Seek(0);
}
  
  
class AbstractCallable {
 public:
  virtual ~AbstractCallable(){};
  virtual void operator()(serializer_type &result, serializer_type &params) = 0;
};
};
};

#endif
