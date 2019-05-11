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

namespace fetch {
namespace memory {

class TrivialRange
{
public:
  using size_type = std::size_t;

  TrivialRange(size_type const &from, size_type const &to)
    : from_(from)
    , to_(to)
  {}

  size_type const &from() const
  {
    return from_;
  }
  size_type const &to() const
  {
    return to_;
  }
  size_type step() const
  {
    return 1;
  }

  template <std::size_t S>
  size_type SIMDFromLower() const
  {
    size_type G = from_ / S;
    return G * S;
  }

  template <std::size_t S>
  size_type SIMDFromUpper() const
  {
    size_type G = from_ / S;
    if (G * S < from_)
    {
      ++G;
    }
    return G * S;
  }

  template <std::size_t S>
  size_type SIMDToLower() const
  {
    size_type G = to_ / S;
    return G * S;
  }

  template <std::size_t S>
  size_type SIMDToUpper() const
  {
    size_type G = to_ / S;
    if (G * S < to_)
    {
      ++G;
    }
    return G * S;
  }

private:
  size_type from_ = 0, to_ = 0;
};

class Range
{
public:
  using size_type = std::size_t;

  Range(size_type const &from = 0, size_type const &to = size_type(-1), size_type const &step = 1)
    : from_(from)
    , to_(to)
    , step_(step)
  {}

  size_type const &from() const
  {
    return from_;
  }
  size_type const &to() const
  {
    return to_;
  }
  size_type const &step() const
  {
    return step_;
  }

  bool is_trivial() const
  {
    return (step_ == 1);
  }

  bool is_undefined() const
  {
    return ((from_ == 0) && (to_ == size_type(-1)));
  }

  TrivialRange ToTrivialRange(size_type const &size) const
  {
    return TrivialRange(from_, std::min(size, to_));
  }

private:
  size_type from_ = 0, to_ = 0, step_ = 1;
};

}  // namespace memory
}  // namespace fetch
