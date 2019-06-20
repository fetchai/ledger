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

#include "core/logger.hpp"

#include <cstdint>
#include <exception>
#include <string>

namespace fetch {
namespace byte_array {
class ConstByteArray;
}  // namespace byte_array
namespace serializers {

namespace error {
using error_type            = uint64_t;
error_type const TYPE_ERROR = 0;
}  // namespace error

class SerializableException : public std::exception
{
public:
  /// @name Construction / Destruction
  /// @{
  SerializableException();
  explicit SerializableException(std::string explanation);
  explicit SerializableException(byte_array::ConstByteArray const &explanation);
  SerializableException(error::error_type error_code, std::string explanation);
  SerializableException(error::error_type                 error_code,
                        byte_array::ConstByteArray const &explanation);
  ~SerializableException() override;
  /// @}

  char const *what() const noexcept override;
  uint64_t    error_code() const;
  std::string explanation() const;

  void StackTrace() const;

private:
  uint64_t    error_code_;
  std::string explanation_;

  LOG_CONTEXT_VARIABLE(stack_trace_)
};

}  // namespace serializers
}  // namespace fetch
