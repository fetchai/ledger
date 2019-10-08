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

#include "core/logging.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-Wsign-compare"
#endif

#ifdef FETCH_ENABLE_BACKTRACE_WITH_DW

#define BACKWARD_HAS_DW 1

#endif

#include "backward.hpp"

#include <csignal>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#define ERROR_BACKTRACE                                         \
  {                                                             \
    std::ostringstream trace;                                   \
                                                                \
    backward::StackTrace st; st.load_here(32); \
    backward::Printer p; \
    p.object = true; \
    p.color_mode = backward::ColorMode::always; \
    p.address = true; \
    p.print(st, trace); \
    \
    FETCH_LOG_INFO(LOGGING_NAME, "Trace: \n", trace.str());     \
  }

