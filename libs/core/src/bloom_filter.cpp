//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/bitvector.hpp"
#include "core/bloom_filter.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <cstddef>
#include <functional>
#include <vector>

namespace fetch {

constexpr std::size_t const INITIAL_SIZE_IN_BITS = 8 * 1024 * 1024;

const auto direct = [](BloomFilter::InputBytes const &input) -> std::vector<std::size_t> {
  auto start = reinterpret_cast<std::size_t const *>(input.pointer());

  const auto               size_in_bytes = input.size();
  std::vector<std::size_t> output(size_in_bytes / sizeof(std::size_t));

  for (std::size_t i = 0; i < output.size(); ++i)
  {
    output[i] = *(start + i);
  }

  return output;
};

BloomFilter::~BloomFilter() = default;

BloomFilter::BloomFilter()
  : bits_(INITIAL_SIZE_IN_BITS)
  , hash_functions_({direct})
{}

bool BloomFilter::Match(InputBytes const &) const
{
  return false;
}

void BloomFilter::Add(InputBytes const &)
{}

}  // namespace fetch
