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

#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

namespace fetch {
namespace telemetry {

/**
 * Get reference to singleton instance
 * @return The reference to the registry
 */
Registry &Registry::Instance()
{
  static Registry instance;
  return instance;
}

/**
 * Ensure the name consists of lowercase letters and underscores
 *
 * @param name The name to be checked
 * @return true if valid, otherwise false
 */
bool Registry::ValidateName(std::string const &name)
{
  bool valid{true};

  for (char c : name)
  {
    valid = (c == '_') || ((c >= 'a') && (c <= 'z'));

    // exit as soon as it it not valid
    if (!valid)
    {
      break;
    }
  }

  return valid;
}

/**
 * Create a counter instance
 *
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 * @return The pointer to the created metric if successful, otherwise a nullptr
 */
CounterPtr Registry::CreateCounter(std::string name, std::string description, Labels labels)
{
  CounterPtr counter{};

  if (ValidateName(name))
  {
    // create the new counter
    counter = std::make_shared<Counter>(std::move(name), std::move(description), std::move(labels));

    // add the counter to the register
    {
      LockGuard guard(lock_);
      measurements_.push_back(counter);
    }
  }

  return counter;
}

/**
 * Collect up all the metrics into a single stream to be presented to the requestor
 *
 * @param stream The reference to the stream to be populated
 */
void Registry::Collect(std::ostream &stream)
{
  LockGuard guard(lock_);
  for (auto const &measurement : measurements_)
  {
    measurement->ToStream(stream);
  }
}

}  // namespace telemetry
}  // namespace fetch
