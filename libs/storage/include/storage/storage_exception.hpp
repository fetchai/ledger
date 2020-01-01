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

#include "core/byte_array/const_byte_array.hpp"
#include "logging/logging.hpp"
#include "storage/storage_exception.hpp"

#include <exception>
#include <string>
#include <utility>

namespace fetch {
namespace storage {

namespace error {
using ErrorType            = uint64_t;
ErrorType const TYPE_ERROR = 0;
}  // namespace error

/**
 * Exceptions for storage-related errors
 */
class StorageException : public std::exception
{
public:
  StorageException()
    : error_code_(error::TYPE_ERROR)
    , explanation_("unknown")
  {}

  explicit StorageException(char const *explanation)
    : error_code_(error::TYPE_ERROR)
    , explanation_(std::string(explanation))
  {}

  explicit StorageException(std::string explanation)
    : error_code_(error::TYPE_ERROR)
    , explanation_(std::move(explanation))
  {}

  explicit StorageException(byte_array::ConstByteArray const &explanation)
    : error_code_(error::TYPE_ERROR)
    , explanation_(std::string(explanation))
  {}

  StorageException(error::ErrorType error_code, std::string explanation)
    : error_code_(error_code)
    , explanation_(std::move(explanation))
  {}

  StorageException(error::ErrorType error_code, byte_array::ConstByteArray const &explanation)
    : error_code_(error_code)
    , explanation_(std::string(explanation))
  {}

  StorageException(StorageException const &) = default;
  StorageException &operator=(StorageException const &) = default;

  StorageException(StorageException &&) noexcept = default;
  StorageException &operator=(StorageException &&) noexcept = default;

  ~StorageException() override = default;

  char const *what() const noexcept override
  {
    return explanation_.c_str();
  }

  uint64_t error_code() const
  {
    return error_code_;
  }

private:
  uint64_t    error_code_;
  std::string explanation_;
};

}  // namespace storage
}  // namespace fetch
