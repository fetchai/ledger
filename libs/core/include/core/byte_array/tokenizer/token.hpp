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

namespace fetch {
namespace byte_array {

class Token : public ConstByteArray
{
public:
  Token()
    : ConstByteArray()
  {}

  Token(char const *str)
    : ConstByteArray(str)
  {}

  Token(std::string const &str)
    : ConstByteArray(str.c_str())
  {}

  Token(ConstByteArray const &other)
    : ConstByteArray(other)
  {}
  Token(ConstByteArray &&other)
    : ConstByteArray(other)
  {}

  Token(ConstByteArray const &other, std::size_t const &start, std::size_t const &length)
    : ConstByteArray(other, start, length)
  {}

  bool operator==(ConstByteArray const &other) const
  {
    return ConstByteArray::operator==(other);
  }

  bool operator!=(ConstByteArray const &other) const
  {
    return !(*this == other);
  }

  void SetType(int const &t)
  {
    type_ = t;
  }
  void SetLine(int const &l)
  {
    line_ = l;
  }
  void SetChar(std::size_t const &c)
  {
    char_ = c;
  }

  int type() const
  {
    return type_;
  }
  int line() const
  {
    return line_;
  }
  std::size_t character() const
  {
    return char_;
  }

private:
  int         type_ = -1;
  int         line_ = 0;
  std::size_t char_ = 0;
};
}  // namespace byte_array
}  // namespace fetch
