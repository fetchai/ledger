#pragma once
#include "core/byte_array/const_byte_array.hpp"

#include <cstdint>

namespace fetch {
namespace beacon {

struct Entropy
{
  using ConstByteArray = byte_array::ConstByteArray;

  uint64_t       round{0};
  uint64_t       number{0};
  ConstByteArray seed{"genesis"};
  ConstByteArray entropy{};
};

}  // namespace beacon
}  // namespace fetch