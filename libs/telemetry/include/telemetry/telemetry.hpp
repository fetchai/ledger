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

#include <memory>

namespace fetch {
namespace telemetry {

class Measurement;
class Counter;
class CounterMap;
class Histogram;
class HistogramMap;

template <typename T>
class Gauge;

using CounterPtr      = std::shared_ptr<Counter>;
using CounterMapPtr   = std::shared_ptr<CounterMap>;
using HistogramPtr    = std::shared_ptr<Histogram>;
using HistogramMapPtr = std::shared_ptr<HistogramMap>;

template <typename T>
using GaugePtr = std::shared_ptr<Gauge<T>>;

}  // namespace telemetry
}  // namespace fetch
