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

#include <chrono>
#include <string>
#include <unordered_map>

#include "core/byte_array/byte_array.hpp"

namespace fetch {
namespace dmlf {
namespace colearn {

class ColearnUpdate
{
public:
  using Algorithm   = std::string;
  using UpdateType  = std::string;
  using Data        = byte_array::ConstByteArray;
  using Source      = std::string;
  using MetaKey     = std::string;
  using MetaValue   = std::string;
  using Metadata    = std::unordered_map<MetaKey, MetaValue>;
  using Resolution  = std::chrono::nanoseconds;
  using Fingerprint = byte_array::ConstByteArray;

  ColearnUpdate(Algorithm algorithm, UpdateType update_type, Data &&data, Source source,
                Metadata metadata);

  Algorithm const &algorithm() const
  {
    return algorithm_;
  }
  UpdateType const &update_type() const
  {
    return update_type_;
  }
  Data const &data() const
  {
    return data_;
  }
  Source const &source() const
  {
    return source_;
  }
  Metadata const &metadata() const
  {
    return metadata_;
  }

  Resolution TimeSinceCreation() const;

  Fingerprint const &fingerprint() const
  {
    return fingerprint_;
  }

private:
  using Clock     = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  Fingerprint ComputeFingerprint();

  Algorithm   algorithm_;
  UpdateType  update_type_;
  Data        data_;
  Source      source_;
  Metadata    metadata_;
  TimePoint   creation_;
  Fingerprint fingerprint_;
};

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
