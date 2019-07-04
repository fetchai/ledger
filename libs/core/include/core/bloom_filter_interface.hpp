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

#include <cstddef>
#include <memory>

namespace fetch {

namespace byte_array {
class ConstByteArray;
}

class BloomFilterInterface
{
public:
  using Bytes = fetch::byte_array::ConstByteArray;

  enum class Type
  {
    DUMMY,
    BASIC
  };

  static std::unique_ptr<BloomFilterInterface> Create(Type);

  virtual ~BloomFilterInterface() = default;

  /*
   * Query the Bloom filter for a given entry.
   *
   * Returns false if the entry is definitely absent; true otherwise.
   */
  virtual bool Match(Bytes const &) = 0;

  /*
   * Add a new entry to the filter.
   */
  virtual void Add(Bytes const &) = 0;

  /*
   * Clients may use this to report how many false positives they identified.
   * This information is used internally by the filter to keep track of the
   * false positive rate.
   *
   * Return false if the filter's measured false positive rate exceeds its
   * target value and rebuilding the filter may be advisable; true otherwise.
   */
  virtual bool ReportFalsePositives(std::size_t) = 0;
};

}  // namespace fetch
