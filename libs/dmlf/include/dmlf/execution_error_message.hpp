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

#include <string>

namespace fetch {
namespace dmlf {

class ExecutionErrorMessage
{
public:
  enum class Stage
  {
    ENGINE = 10,
    COMPILE,
    RUNNING,
    NETWORK,
  };

  enum class Code
  {
    SUCCESS = 0,

    BAD_TARGET = 100,
    BAD_EXECUTABLE,
    BAD_STATE,
    BAD_DESTINATION,
    
    COMPILATION_ERROR,
    RUNTIME_ERROR
  };

  explicit ExecutionErrorMessage(Stage stage, Code code, std::string const &message )
  :stage_(stage), code_(code), message_(message)
  {
  }

  Stage stage() const
  {
    return stage_;
  }
  Code code() const
  {
    return code_;
  }
  std::string message()  const
  {
    return message_;
  }

private:
  Stage stage_;
  Code code_;
  std::string message_;
};

}  // namespace dmlf
}  // namespace fetch
