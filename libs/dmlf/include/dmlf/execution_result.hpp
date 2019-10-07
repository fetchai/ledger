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

#include "vm/variant.hpp"
#include "dmlf/execution_error_message.hpp"

namespace fetch {
namespace dmlf {

class ExecutionResult
{
public:
  using Variant = fetch::vm::Variant;
  using Error = ExecutionErrorMessage;

  ExecutionResult(Variant output, Error error, std::string console)
  :output_(output), error_(error), console_(console)
  {
  }

  Variant output() const
  {
    return output_;
  }
  Error error() const
  {
    return error_;
  }
  std::string console()  const
  {
    return console_;
  }

private:
  Variant output_;
  Error error_;
  std::string console_;
};

}  // namespace dmlf
}  // namespace fetch
