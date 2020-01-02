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

#include "telemetry/counter.hpp"
#include "telemetry/counter_map.hpp"

namespace fetch {
namespace telemetry {

CounterMap::CounterMap(std::string name, std::string description, Labels const &labels)
  : Measurement(std::move(name), std::move(description), labels)
{}

void CounterMap::Increment(Labels const &keys)
{
  LookupCounter(keys)->increment();
}

void CounterMap::ToStream(OutputStream &stream) const
{
  std::lock_guard<std::mutex> guard(lock_);

  WriteHeader(stream, "counter");
  for (auto const &element : counters_)
  {
    element.second->ToStream(stream);
  }
}

CounterPtr CounterMap::LookupCounter(Labels const &keys)
{
  std::lock_guard<std::mutex> guard(lock_);

  auto it = counters_.find(keys);
  if (it == counters_.end())
  {
    // create the new keys from the base labels merged with the input keys
    Labels new_labels = labels();
    for (auto const &element : keys)
    {
      new_labels[element.first] = element.second;
    }

    // create the counter
    auto counter = std::make_shared<Counter>(name(), "", std::move(new_labels));

    // store the counter
    counters_[keys] = counter;

    return counter;
  }

  return it->second;
}

}  // namespace telemetry
}  // namespace fetch
