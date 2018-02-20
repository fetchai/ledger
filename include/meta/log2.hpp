#ifndef META_LOG2_HPP
#define META_LOG2_HPP

#include <cstdint>
namespace details {
namespace meta {

template <uint64_t N>
struct Log2 {
  enum { value = 1 + Log2<(N >> 1)>::value };
};
template <>
struct Log2<1> {
  enum { value = 0 };
};


};
};
#endif
