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

#include "moment/clocks.hpp"
#include "core/mutex.hpp"
#include "moment/detail/adjustable_clock.hpp"
#include "moment/detail/steady_clock.hpp"

#include <string>
#include <unordered_map>

namespace fetch {
namespace moment {
namespace {

using Mutex      = mutex::Mutex;
using ClockStore = std::unordered_map<std::string, ClockPtr>;

mutex::Mutex clock_store_lock{__LINE__, __FILE__};
ClockStore   clock_store;

/**
 * Create a normal version of the clock
 *
 * @param type The clock type to create
 * @return The constructed clock type if successful, otherwise false
 */
ClockPtr CreateNormal(ClockType type)
{
  ClockPtr clock{};

  switch (type)
  {
  case ClockType::STEADY:
    clock = std::make_shared<detail::SteadyClock>();
    break;
  }

  return clock;
}

/**
 * Create an adjustable version of the clock
 *
 * @param type The clock type to create
 * @return The instance of the clock if successful, otherwise false
 */
AdjustableClockPtr CreateAdjustable(ClockType type)
{
  AdjustableClockPtr clock{};

  switch (type)
  {
  case ClockType::STEADY:
    clock = std::make_shared<detail::AdjustableClock<detail::SteadyClock>>();
    break;
  }

  return clock;
}
}  // namespace

/**
 * Create or lookup a requested clock
 *
 * @param name The name of the clock
 * @param default_type The default type of the clock
 * @return The requested clock if successful, otherwise a null pointer
 */
ClockPtr GetClock(char const *name, ClockType default_type)
{
  std::string const clock_name{name};

  FETCH_LOCK(clock_store_lock);

  // if the clock already exists then look it up
  auto it = clock_store.find(name);
  if (it != clock_store.end())
  {
    return it->second;
  }

  // create the new clock
  auto clock = CreateNormal(default_type);

  // add the clock to the store if one was successfully creates
  if (clock)
  {
    clock_store[clock_name] = clock;
  }

  return clock;
}

/**
 * Create an adjustable version of the requested clock type
 *
 * @param name The name of the clock
 * @param type The type of the clock to be successful
 * @return the instance to the clock if successful, otherwise a null pointer
 */
AdjustableClockPtr CreateAdjustableClock(char const *name, ClockType type)
{
  std::string const clock_name{name};

  FETCH_LOCK(clock_store_lock);

  // create the new clock
  auto clock = CreateAdjustable(type);

  // add the clock to the store if one was successfully creates
  if (clock)
  {
    clock_store[clock_name] = clock;
  }

  return clock;
}

}  // namespace moment
}  // namespace fetch