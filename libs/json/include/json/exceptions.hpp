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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/tokenizer/token.hpp"

#include <exception>
#include <string>
#include <utility>

namespace fetch {
namespace json {

class JSONParseException : public std::exception
{
public:
  explicit JSONParseException(std::string err)
    : error_(std::move(err))
  {}

  JSONParseException(JSONParseException const &) = default;
  JSONParseException &operator=(JSONParseException const &) = default;

  JSONParseException(JSONParseException &&) noexcept = default;
  JSONParseException &operator=(JSONParseException &&) noexcept = default;

  ~JSONParseException() override = default;

  char const *what() const noexcept override
  {
    return error_.c_str();
  }

private:
  std::string error_;
};
}  // namespace json
}  // namespace fetch
