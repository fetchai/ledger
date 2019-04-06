#pragma once
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

#include <iomanip>
#include <sstream>

namespace fetch {

static constexpr std::size_t MAX_THREAD_NAME_LEN = 16;  // posix limit

inline void SetThreadName(std::string name)
{
  if (name.size() > MAX_THREAD_NAME_LEN)
  {
    name = name.substr(0, MAX_THREAD_NAME_LEN);
  }

#if defined(FETCH_PLATFORM_MACOS)
  // mac
  pthread_setname_np(name.c_str());
#elif defined(FETCH_PLATFORM_LINUX)
  // linux / posix
  pthread_setname_np(pthread_self(), name.c_str());
#endif
}

inline void SetThreadName(std::string const &prefix, std::size_t index)
{
  static constexpr std::size_t MAX_INDEX_LEN  = 2;
  static constexpr std::size_t MAX_PREFIX_LEN = MAX_THREAD_NAME_LEN - MAX_INDEX_LEN;

  std::ostringstream oss;

  if (prefix.size() > MAX_PREFIX_LEN)
  {
    oss << prefix.substr(0, MAX_PREFIX_LEN);
  }
  else
  {
    oss << prefix;
  }

  oss << std::setfill('0') << std::setw(MAX_INDEX_LEN) << index;

  // set the actual thread names
  SetThreadName(oss.str());
}

}  // namespace fetch