#pragma once
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

#include <cstddef>
#include <functional>
#include <vector>

namespace fetch {

namespace byte_array {
class ConstByteArray;
}

class BloomFilter
{
public:
  using InputBytes = fetch::byte_array::ConstByteArray;

  BloomFilter();
  ~BloomFilter();

  BloomFilter(BloomFilter const &) = delete;
  BloomFilter(BloomFilter &&)      = delete;
  BloomFilter &operator=(BloomFilter const &) = delete;
  BloomFilter &operator=(BloomFilter &&) = delete;

  bool Match(InputBytes const &element) const;
  void Add(InputBytes const &element);

private:
  using Function  = std::function<std::vector<std::size_t>(InputBytes const &)>;
  using Functions = std::vector<Function>;

  BitVector       bits_;
  Functions const hash_functions_;
};

}  // namespace fetch
