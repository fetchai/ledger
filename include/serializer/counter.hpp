#ifndef SERIALIZER_COUNTER_HPP
#define SERIALIZER_COUNTER_HPP
#include "byte_array/referenced_byte_array.hpp"

#include <type_traits>

namespace fetch {
namespace serializers {
class SizeCounter {
 public:
  void Allocate(std::size_t const &val) { size_ += val; }

  void WriteBytes(uint8_t const *arr, std::size_t const &size) {}
  void ReadBytes(uint8_t const *arr, std::size_t const &size) {}

  template <typename T>
  SizeCounter &operator<<(T const &val) {
    Serialize(*this, val);
    return *this;
  }

  std::size_t size() const { return size_; }

 private:
  std::size_t size_ = 0;
};
};
};

#endif
