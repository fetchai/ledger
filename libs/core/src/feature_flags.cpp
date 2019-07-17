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

#include "core/feature_flags.hpp"

#include <cstddef>
#include <iostream>

namespace fetch {
namespace core {
namespace {

using fetch::byte_array::ConstByteArray;

ConstByteArray Split(ConstByteArray const &ref, std::size_t &start)
{
  ConstByteArray token{};

  for (std::size_t curr = start; curr < ref.size(); ++curr)
  {
    if (ref[curr] == ',')
    {
      token = ref.SubArray(start, curr - start);
      start = curr + 1;
      break;
    }
  }

  if (token.empty() && start < ref.size())
  {
    token = ref.SubArray(start, ref.size() - start);
    start = ref.size();
  }

  return token;
}

}  // namespace

void FeatureFlags::Parse(ConstByteArray const &contents)
{
  std::size_t start{0};
  for (;;)
  {
    auto token = Split(contents, start);

    // exit once we have reached the end
    if (token.empty())
    {
      break;
    }

    // add the flag to the set
    flags_.emplace(std::move(token));
  }
}

std::ostream &operator<<(std::ostream &stream, core::FeatureFlags const &flags)
{
  bool add_sep{false};

  for (auto const &element : flags)
  {
    if (add_sep)
    {
      stream << ',';
    }

    stream << element;
    add_sep = true;
  }

  return stream;
}

std::istream &operator>>(std::istream &stream, core::FeatureFlags &flags)
{
  std::string value;
  stream >> value;

  if (!stream.fail())
  {
    flags.Parse(value);
  }

  return stream;
}

}  // namespace core
}  // namespace fetch
