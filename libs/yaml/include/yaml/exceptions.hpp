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
#include <sstream>
#include <string>
#include <utility>

namespace fetch {
namespace yaml {

class UnrecognisedYamlSymbolException : public std::exception
{
  std::string str_;

public:
  explicit UnrecognisedYamlSymbolException(byte_array::Token const &token)
  {
    std::stringstream msg;
    msg << "Unrecognised symbol '";
    msg << token << '\'' << " at line " << token.line() << ", character " << token.character()
        << "\n";
    str_ = msg.str();
  }

  char const *what() const noexcept override
  {
    return str_.c_str();
  }
};

class YamlParseException : public std::exception
{
public:
  explicit YamlParseException(std::string err)
    : error_(std::move(err))
  {}

  YamlParseException(YamlParseException const &) = default;
  YamlParseException &operator=(YamlParseException const &) = default;

  YamlParseException(YamlParseException &&) noexcept = default;
  YamlParseException &operator=(YamlParseException &&) noexcept = default;

  ~YamlParseException() override = default;

  char const *what() const noexcept override
  {
    return error_.c_str();
  }

private:
  std::string error_;
};

}  // namespace yaml
}  // namespace fetch
