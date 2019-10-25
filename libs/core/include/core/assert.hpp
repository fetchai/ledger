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
#include "meta/value_util.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

namespace fetch {
namespace assert {
namespace details {

template <typename... Args>
constexpr void Print(Args &&... args)
{
  value_util::ForEach([](auto &&arg) { std::cerr << std::forward<decltype(arg)>(arg); },
                      std::forward<Args>(args)...);
}

}  // namespace details
}  // namespace assert
}  // namespace fetch

#ifndef FETCH_DISABLE_TODO_COUT

#define TODO_FAIL(...)                                              \
  fetch::assert::details::Print(__VA_ARGS__);                       \
  FETCH_LOG_ERROR("TODO_FAIL", "About to fail.");                   \
  std::cerr << '\n' << __FILE__ << " at line " << __LINE__ << '\n'; \
  throw std::runtime_error("Dependence on non-existing functionality!")

#define TODO(...)                             \
  fetch::assert::details::Print(__VA_ARGS__); \
  std::cerr << '\n' << __FILE__ << " at line " << __LINE__ << '\n'

#else

#define TODO_FAIL(...) throw std::runtime_error("Dependence on non-existing functionality!")
#define TODO(...)

#endif

#define detailed_assert(cond)                                                                    \
  if (!(cond))                                                                                   \
  {                                                                                              \
    std::cout << "Failed :" << #cond << " in " << __FILE__ << " line " << __LINE__ << std::endl; \
    throw std::runtime_error("Assertion failed");                                                \
  }

/*
 * Replace ASSERT(param) with (void)(param) in release mode
 * This is to avoid error due to misc-unused-parameters,-warnings-as-errors
 * when the only place you use a named parameter is in an assert
 */
#ifndef NDEBUG
#define ASSERT assert
#else
#define ASSERT(...) (void)(__VA_ARGS__)
#endif
