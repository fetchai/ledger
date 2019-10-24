#pragma once

#include "semanticsearch/query/query_instruction.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/consumers.hpp"
#include "core/byte_array/tokenizer/tokenizer.hpp"

#include <vector>

namespace fetch {
namespace semanticsearch {

struct Query
{
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  ConstByteArray                 source;
  ConstByteArray                 filename;
  std::vector<CompiledStatement> statements;
};

}  // namespace semanticsearch
}  // namespace fetch