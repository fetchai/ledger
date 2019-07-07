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

#include "core/string/ends_with.hpp"
#include "telemetry/counter.hpp"

#include <iostream>

namespace fetch {
namespace telemetry {
namespace {

bool ValidateName(std::string const &name)
{
  return core::EndsWith(name, "_total");
}

}  // namespace

Counter::Counter(std::string name, std::string description, Labels labels)
  : Measurement(std::move(name), std::move(description), std::move(labels))
{
  if (!ValidateName(this->name()))
  {
    throw std::runtime_error("Incorrect counter name, must end with _total");
  }
}

/**
 * Write the value of the metric to the stream so as to be consumed by external components
 *
 * @param stream The stream to be updated
 * @param mode The mode to be used when generating the stream
 */

void Counter::ToStream(std::ostream &stream, StreamMode mode) const
{
  WriteHeader(stream, "counter", mode);
  WriteValuePrefix(stream) << counter_ << '\n';
}

}  // namespace telemetry
}  // namespace fetch
