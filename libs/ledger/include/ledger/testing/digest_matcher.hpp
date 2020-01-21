#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "ledger/chain/block.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::MatchResultListener;
using ::testing::Matcher;
using ::testing::MatcherInterface;

namespace fetch {
namespace ledger {
namespace testing {

class DigestMatcher : public MatcherInterface<byte_array::ConstByteArray>
{
public:
  using type     = byte_array::ConstByteArray;
  using Patterns = std::unordered_map<type, std::string>;

  explicit DigestMatcher(type expected);
  DigestMatcher(type expected, Patterns const &patterns);

  bool MatchAndExplain(type actual, MatchResultListener *listener) const override;

  void DescribeTo(std::ostream *os) const override;

  template <class... NamesAndContainers>
  static Patterns MakePatterns(NamesAndContainers &&... names_and_containers)
  {
    return KeepPatterns(Patterns{}, std::forward<NamesAndContainers>(names_and_containers)...);
  }

private:
  template <template <class...> class Container, class... ContainerArgs,
            class... NamesAndContainers>
  static Patterns KeepPatterns(
      Patterns patterns, std::string const &name,
      Container<BlockPtr, ContainerArgs...> const &container,
      NamesAndContainers &&... names_and_containers);

  static Patterns KeepPatterns(Patterns patterns);

  static std::string Show(type const &hash);

  template <class Stream>
  void Identify(type const &hash, Stream *stream) const
  {
    auto position = patterns_->find(hash);
    if (position != patterns_->end())
    {
      *stream << "which is at " << position->second;
    }
    else
    {
      *stream << "unknown so far";
    }
  }

  type            expected_;
  Patterns const *patterns_ = nullptr;
};

Matcher<byte_array::ConstByteArray> ExpectedHash(byte_array::ConstByteArray expected);

template <template <class...> class Container, class... ContainerArgs, class... NamesAndContainers>
DigestMatcher::Patterns DigestMatcher::KeepPatterns(
    Patterns patterns, std::string const &name,
    Container<BlockPtr, ContainerArgs...> const &container,
    NamesAndContainers &&... names_and_containers)
{
  std::size_t index{};
  for (auto const &block : container)
  {
    patterns.emplace(block->hash, name + '[' + std::to_string(index++) + ']');
  }
  return KeepPatterns(std::move(patterns),
                      std::forward<NamesAndContainers>(names_and_containers)...);
}

}  // namespace testing
}  // namespace ledger
}  // namespace fetch
