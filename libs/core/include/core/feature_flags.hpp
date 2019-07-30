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

#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"

#include <unordered_set>

namespace fetch {
namespace core {

/**
 * Simple collection of a set of strings to represent features which are enabled
 */
class FeatureFlags
{
public:
  constexpr static char const *MAIN_CHAIN_BLOOM_FILTER = "main_chain_bloom_filter";

  using ConstByteArray = byte_array::ConstByteArray;
  using FlagSet        = std::unordered_set<ConstByteArray>;
  using Iterator       = FlagSet::iterator;
  using ConstIterator  = FlagSet::const_iterator;

  // Construction / Destruction
  FeatureFlags()                         = default;
  FeatureFlags(FeatureFlags const &)     = default;
  FeatureFlags(FeatureFlags &&) noexcept = default;
  ~FeatureFlags()                        = default;

  // Parsing
  void Parse(ConstByteArray const &contents);

  // Queries
  bool IsEnabled(ConstByteArray const &value) const;

  // Iteration
  Iterator      begin();
  ConstIterator begin() const;
  ConstIterator cbegin() const;
  Iterator      end();
  ConstIterator end() const;
  ConstIterator cend() const;
  bool          empty() const;
  std::size_t   size() const;

  // Operators
  FeatureFlags &operator=(FeatureFlags const &) = default;
  FeatureFlags &operator=(FeatureFlags &&) noexcept = default;

  // Streaming Support
  friend std::ostream &operator<<(std::ostream &stream, core::FeatureFlags const &flags);
  friend std::istream &operator>>(std::istream &stream, core::FeatureFlags &flags);

private:
  FlagSet flags_;  ///< Flag storage
};

}  // namespace core
}  // namespace fetch
