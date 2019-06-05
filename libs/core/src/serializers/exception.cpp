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

#include "core/serializers/exception.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"

#include <string>
#include <utility>

namespace fetch {
namespace serializers {

SerializableException::SerializableException()
  : error_code_(error::TYPE_ERROR)
  , explanation_("unknown")
{
  LOG_STACK_TRACE_POINT;

  LOG_SET_CONTEXT_VARIABLE(stack_trace_)
}

SerializableException::SerializableException(std::string explanation)
  : error_code_(error::TYPE_ERROR)
  , explanation_(std::move(explanation))
{
  LOG_STACK_TRACE_POINT;

  LOG_SET_CONTEXT_VARIABLE(stack_trace_)
}

SerializableException::SerializableException(byte_array::ConstByteArray const &explanation)
  : error_code_(error::TYPE_ERROR)
  , explanation_(std::string(explanation))
{
  LOG_STACK_TRACE_POINT;

  LOG_SET_CONTEXT_VARIABLE(stack_trace_)
}

SerializableException::SerializableException(error::error_type error_code, std::string explanation)
  : error_code_(error_code)
  , explanation_(std::move(explanation))
{
  LOG_STACK_TRACE_POINT;

  LOG_SET_CONTEXT_VARIABLE(stack_trace_)
}

SerializableException::SerializableException(error::error_type                 error_code,
                                             byte_array::ConstByteArray const &explanation)
  : error_code_(error_code)
  , explanation_(std::string(explanation))
{
  LOG_STACK_TRACE_POINT;

  LOG_SET_CONTEXT_VARIABLE(stack_trace_)
}

SerializableException::~SerializableException()
{
  LOG_STACK_TRACE_POINT;
}

char const *SerializableException::what() const noexcept
{
  return explanation_.c_str();
}

uint64_t SerializableException::error_code() const
{
  return error_code_;
}

std::string SerializableException::explanation() const
{
  return explanation_;
}

void SerializableException::StackTrace() const
{
  LOG_PRINT_STACK_TRACE(stack_trace_, "Trace at time of exception")
}

}  // namespace serializers
}  // namespace fetch
