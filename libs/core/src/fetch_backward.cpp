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

#include "core/fetch_backward.hpp"

//#ifdef FETCH_ENABLE_BACKTRACE

// Add functionality to print a stack trace when program-terminating signals such as sigsegv are
// found
std::function<void(std::string)> backward::SignalHandling::_on_signal;
backward::SignalHandling         sh([](std::string const &fatal_msg) {
  FETCH_LOG_ERROR("FETCH_FATAL_SIGNAL_HANDLER", fatal_msg);
});

//#endif
