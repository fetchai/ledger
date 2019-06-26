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
  using ConstByteArray = byte_array::ConstByteArray;
  using FlagSet        = std::unordered_set<ConstByteArray>;
  using Iterator       = FlagSet::iterator;
  using ConstIterator  = FlagSet::const_iterator;

  // Construction / Destruction
  FeatureFlags()                         = default;
  FeatureFlags(FeatureFlags const &)     = delete;
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
  FeatureFlags &operator=(FeatureFlags const &) = delete;
  FeatureFlags &operator=(FeatureFlags &&) noexcept = default;

private:
  FlagSet flags_;  ///< Flag storage
};

/**
 * Check to see if a feature is enabled (present)
 *
 * @param value The feature being queried
 * @return true if present, otherwise false
 */
inline bool FeatureFlags::IsEnabled(ConstByteArray const &value) const
{
  return flags_.find(value) != flags_.end();
}

/**
 * Return the iterator to the beginning of the feature set
 *
 * @return The iterator to the beginning
 */
inline FeatureFlags::Iterator FeatureFlags::begin()
{
  return flags_.begin();
}

/**
 * Return the const iterator to the beginning of the feature set
 *
 * @return The const iterator to the beginning
 */
inline FeatureFlags::ConstIterator FeatureFlags::begin() const
{
  return flags_.begin();
}

/**
 * Return the const iterator to the beginning of the feature set
 *
 * @return The const iterator to the beginning
 */
inline FeatureFlags::ConstIterator FeatureFlags::cbegin() const
{
  return flags_.cbegin();
}

/**
 * Return the iterator to the end of the feature set
 *
 * @return The iterator to the end
 */
inline FeatureFlags::Iterator FeatureFlags::end()
{
  return flags_.end();
}

/**
 * Return the const iterator to the end of the feature set
 *
 * @return The const iterator to the end
 */
inline FeatureFlags::ConstIterator FeatureFlags::end() const
{
  return flags_.end();
}

/**
 * Return the const iterator to the end of the feature set
 *
 * @return The const iterator to the end
 */
inline FeatureFlags::ConstIterator FeatureFlags::cend() const
{
  return flags_.cend();
}

/**
 * Determine if the current flag set is empty
 *
 * @return true if there are no flags otherwise false
 */
inline bool FeatureFlags::empty() const
{
  return flags_.empty();
}

/**
 * Return the number of
 * @return
 */
inline std::size_t FeatureFlags::size() const
{
  return flags_.size();
}

}  // namespace core
}  // namespace fetch
