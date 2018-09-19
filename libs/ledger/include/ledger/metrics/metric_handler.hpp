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

#include "core/byte_array/const_byte_array.hpp"

namespace fetch {
namespace ledger {

class MetricHandler
{
public:
  using Clock          = std::chrono::high_resolution_clock;
  using Timestamp      = Clock::time_point;
  using ConstByteArray = byte_array::ConstByteArray;

  enum class Instrument
  {
    TRANSACTION
  };

  enum class Event
  {
    SUBMITTED,
    QUEUED,
    PACKED,
    EXECUTION_STARTED,
    EXECUTION_COMPLETE
  };

  // Construction / Destruction
  MetricHandler()          = default;
  virtual ~MetricHandler() = default;

  virtual void RecordMetric(ConstByteArray const &identifier, Instrument instrument, Event event,
                            Timestamp const &timestamp) = 0;
};

}  // namespace ledger
}  // namespace fetch
