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

#include <stdexcept>
#include <string>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * Exception class that is generated in response to loading and running Smart Contracts
 */
class SmartContractException : public std::exception
{
public:
  enum class Category
  {
    UNKNOWN,
    COMPILATION
  };

  using Errors = std::vector<std::string>;

  SmartContractException(Category category, Errors errors)
    : category_{category}
    , errors_{std::move(errors)}
  {}

  Errors const &errors() const
  {
    return errors_;
  }
  Category category() const
  {
    return category_;
  }

  const char *what() const noexcept override
  {
    if (errors_.empty())
    {
      return "Unknown Smart Contract Error";
    }
    else
    {
      return errors_.front().c_str();
    }
  }

private:
  Category category_{Category::UNKNOWN};
  Errors   errors_{};
};

}  // namespace ledger
}  // namespace fetch