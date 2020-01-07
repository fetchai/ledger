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
#include "core/byte_array/encoders.hpp"

#include <cstdint>

namespace fetch {
namespace dmlf {

class UpdateInterface
{
public:
  using TimeStampType = std::uint64_t;
  using Fingerprint   = byte_array::ByteArray;

  UpdateInterface() = default;

  virtual ~UpdateInterface() = default;

  // API
  virtual byte_array::ByteArray Serialise()                                = 0;
  virtual byte_array::ByteArray Serialise(std::string type)                = 0;
  virtual void                  DeSerialise(const byte_array::ByteArray &) = 0;
  virtual TimeStampType         TimeStamp() const                          = 0;
  virtual Fingerprint           GetFingerprint() const                     = 0;
  virtual std::string           DebugString() const
  {
    return static_cast<std::string>(byte_array::ToBase64(Fingerprint())) + "@" +
           std::to_string(TimeStamp());
  }

  // Queue ordering
  bool operator>(UpdateInterface const &other) const
  {
    return TimeStamp() > other.TimeStamp();
  }

  UpdateInterface(UpdateInterface const &other) = delete;
  UpdateInterface &operator=(UpdateInterface const &other)  = delete;
  bool             operator==(UpdateInterface const &other) = delete;
  bool             operator<(UpdateInterface const &other)  = delete;

protected:
private:
};

using deprecated_UpdateInterfacePtr = std::shared_ptr<UpdateInterface>;

}  // namespace dmlf
}  // namespace fetch
