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

#include "core/mutex.hpp"

#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>

namespace fetch {

template <typename U>
CustomMutex<U>::CustomMutex(char const *file, int line)
  : file_{file}
  , line_{line}
{}

template <typename U>
void CustomMutex<U>::lock()
{
  RecordEvent(Event::WAIT_FOR_LOCK);
  U::lock();
  RecordEvent(Event::LOCKED);
}

template <typename U>
bool CustomMutex<U>::try_lock()
{
  bool const success = U::try_lock();

  if (success)
  {
    RecordEvent(Event::LOCKED);
  }

  return success;
}

template <typename U>
void CustomMutex<U>::unlock()
{
  U::unlock();
  RecordEvent(Event::UNLOCKED);
}

template <typename U>
char const *CustomMutex<U>::ToString(Event event)
{
  char const *text = "Unknown";

  switch (event)
  {
  case Event::WAIT_FOR_LOCK:
    text = "Waiting for lock";
    break;
  case Event::LOCKED:
    text = "Locked";
    break;
  case Event::UNLOCKED:
    text = "Unlocked";
    break;
  }

  return text;
}

template <typename U>
void CustomMutex<U>::RecordEvent(Event event)
{
  using Clock = std::chrono::high_resolution_clock;

  // build up the filename
  std::ostringstream file_name_buffer;
  file_name_buffer << "thread_" << std::this_thread::get_id() << ".sync-events.db";

  // build up the data payload
  std::ostringstream payload_buffer{};
  bool first_time{true};

  auto add_to_payload = [&first_time, &payload_buffer](char const *name, auto const &value) {
    if (!first_time)
    {
      payload_buffer << ',';
    }

    payload_buffer << '"' << name << "\": \"" << value << '"';

    first_time = false;
  };

  auto const timestamp = Clock::now().time_since_epoch().count();

  payload_buffer << '{';
  add_to_payload("event", ToString(event));
  add_to_payload("event_code", static_cast<int>(event));
  add_to_payload("timestamp", timestamp);
  add_to_payload("file", file_);
  add_to_payload("line", line_);
  add_to_payload("thread", std::this_thread::get_id());
  add_to_payload("instance", this);
  payload_buffer << '}';

  // save and flush the stats to disk
  std::ofstream output_file{file_name_buffer.str().c_str(), std::ios::out | std::ios::app};
  output_file << payload_buffer.str() << std::endl;
}

template class CustomMutex<std::mutex>;
template class CustomMutex<std::recursive_mutex>;

} // namespace fetch