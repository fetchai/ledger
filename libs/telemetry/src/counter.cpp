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

#include <iostream>

namespace fetch {
namespace telemetry {

//# HELP go_memstats_lookups_total Total number of pointer lookups.
//# TYPE go_memstats_lookups_total counter
// go_memstats_lookups_total 0

bool Counter::ToStream(std::ostream &stream) const
{
  stream << "# HELP " << name() << ' ' << description() << '\n'
         << "# TYPE " << name() << " counter\n"
         << name() << ' ' << counter_ << '\n';

  return true;
}

}  // namespace telemetry
}  // namespace fetch
