#ifndef VECTORIZE_INFO_HPP
#define VECTORIZE_INFO_HPP
#include <cstdint>
#include <cstddef>


namespace fetch {
namespace vectorize {


template< typename T, std::size_t >
struct VectorInfo {
  typedef T naitve_type;
  typedef T register_type;
};


};
};

#endif
