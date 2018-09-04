#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/logger.hpp"
#include <exception>
#include <string>
#include <utility>

namespace fetch {
namespace swarm {

class SwarmException : public std::exception
{
public:
  SwarmException(std::string explanation) : explanation_(std::move(explanation))
  {
    LOG_STACK_TRACE_POINT;
    LOG_SET_CONTEXT_VARIABLE(stack_trace_);
  }

  virtual ~SwarmException()
  {
    LOG_STACK_TRACE_POINT;
  }

  char const *what() const noexcept override
  {
    return explanation_.c_str();
  }
  uint64_t error_code() const
  {
    return 1;
  }
  std::string explanation() const
  {
    return explanation_;
  }

  void StackTrace() const
  {
    LOG_PRINT_STACK_TRACE(stack_trace_, "Trace at time of exception");
  }

private:
  std::string explanation_;

  LOG_CONTEXT_VARIABLE(stack_trace_)
};

}  // namespace swarm
}  // namespace fetch
