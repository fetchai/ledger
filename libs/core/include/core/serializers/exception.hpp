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

#include <cstdint>
#include <exception>
#include <string>

namespace fetch {
namespace byte_array {
class ConstByteArray;
}  // namespace byte_array
namespace serializers {

namespace error {
using ErrorType            = uint64_t;
ErrorType const TYPE_ERROR = 0;
}  // namespace error

class SerializableException : public std::exception
{
public:
  /// @name Construction / Destruction
  /// @{
  SerializableException();
  explicit SerializableException(std::string explanation);
  explicit SerializableException(byte_array::ConstByteArray const &explanation);
  SerializableException(error::ErrorType error_code, char const *explanation);
  SerializableException(error::ErrorType error_code, std::string explanation);
  SerializableException(error::ErrorType error_code, byte_array::ConstByteArray const &explanation);
  SerializableException(SerializableException const &)     = default;
  SerializableException(SerializableException &&) noexcept = default;
  ~SerializableException() override                        = default;
  /// @}

  char const *what() const noexcept override;
  uint64_t    error_code() const;
  std::string explanation() const;

  // Assignment Operators
  SerializableException &operator=(SerializableException const &) = default;
  SerializableException &operator=(SerializableException &&) noexcept = default;

private:
  uint64_t    error_code_;
  std::string explanation_;
};

}  // namespace serializers
}  // namespace fetch
