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
  using SizeType = std::size_t;

  TrivialRange(SizeType const &from, SizeType const &to)
    : from_(from)
    , to_(to)
  {}

  SizeType const &from() const
  {
    return from_;
  }
  SizeType const &to() const
  {
    return to_;
  }
  SizeType step() const
  {
    return 1;
  }

  template <std::size_t S>
  SizeType SIMDFromLower() const
  {
    SizeType G = from_ / S;
    return G * S;
  }

  template <std::size_t S>
  SizeType SIMDFromUpper() const
  {
    SizeType G = from_ / S;
    if (G * S < from_)
    {
      ++G;
    }
    return G * S;
  }

  template <std::size_t S>
  SizeType SIMDToLower() const
  {
    SizeType G = to_ / S;
    return G * S;
  }

  template <std::size_t S>
  SizeType SIMDToUpper() const
  {
    SizeType G = to_ / S;
    if (G * S < to_)
    {
      ++G;
    }
    return G * S;
  }

private:
  SizeType from_ = 0, to_ = 0;
};

class Range
{
public:
  using SizeType = std::size_t;

  Range(SizeType const &from = 0, SizeType const &to = SizeType(-1), SizeType const &step = 1)
    : from_(from)
    , to_(to)
    , step_(step)
  {}

  SizeType const &from() const
  {
    return from_;
  }
  SizeType const &to() const
  {
    return to_;
  }
  SizeType const &step() const
  {
    return step_;
  }

  bool is_trivial() const
  {
    return (step_ == 1);
  }

  bool is_undefined() const
  {
    return ((from_ == 0) && (to_ == SizeType(-1)));
  }

  TrivialRange ToTrivialRange(SizeType const &size) const
  {
    return TrivialRange(from_, std::min(size, to_));
  }

private:
  SizeType from_ = 0, to_ = 0, step_ = 1;
};

}  // namespace memory
}  // namespace fetch
