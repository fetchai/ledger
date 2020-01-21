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

#include "ledger/testing/digest_matcher.hpp"

#include <gtest/gtest.h>

using ::testing::MatchResultListener;
using ::testing::MakeMatcher;
using ::testing::Matcher;

namespace fetch {
namespace ledger {
namespace testing {

DigestMatcher::DigestMatcher(type expected)
  : expected_(std::move(expected))
{}

DigestMatcher::DigestMatcher(type expected, Patterns const &patterns)
  : expected_(std::move(expected))
  , patterns_(&patterns)
{}

bool DigestMatcher::MatchAndExplain(type actual, MatchResultListener *listener) const
{
  if (actual == expected_)
  {
    return true;
  }
  *listener << Show(actual);
  if (static_cast<bool>(patterns_))
  {
    Identify(actual, listener);
  }
  return false;
}

void DigestMatcher::DescribeTo(std::ostream *os) const
{
  *os << Show(expected_);
  if (static_cast<bool>(patterns_))
  {
    *os << ", ";
    Identify(expected_, os);
  }
}

DigestMatcher::Patterns DigestMatcher::KeepPatterns(Patterns patterns)
{
  return patterns;
}

std::string DigestMatcher::Show(type const &hash)
{
  return std::string(hash.ToHex().SubArray(0, 8));
}

Matcher<byte_array::ConstByteArray> ExpectedHash(byte_array::ConstByteArray expected)
{
  return MakeMatcher(new DigestMatcher(std::move(expected)));
}

}  // namespace testing
}  // namespace ledger
}  // namespace fetch
