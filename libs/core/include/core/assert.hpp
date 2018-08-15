#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include <iostream>
#include <stdexcept>
namespace fetch {
namespace assert {
namespace details {
struct Printer
{
  template <typename T, typename... Args>
  static void Print(T const &next, Args... args)
  {
    std::cerr << next;
    Print(args...);
  }

  template <typename T>
  static void Print(T const &next)
  {
    std::cerr << next;
  }
};
}  // namespace details
}  // namespace assert
}  // namespace fetch

#ifndef FETCH_DISABLE_TODO_COUT
// TODO: Rename to FETCH_...
#define TODO_FAIL(...)                                                        \
  fetch::assert::details::Printer::Print(__VA_ARGS__);                        \
  std::cerr << std::endl << __FILE__ << " at line " << __LINE__ << std::endl; \
  throw std::runtime_error("Dependence on non-existing functionality!");

#define TODO(...)                                      \
  fetch::assert::details::Printer::Print(__VA_ARGS__); \
  std::cerr << std::endl << __FILE__ << " at line " << __LINE__ << std::endl;

#else

#define TODO_FAIL(...) throw std::runtime_error("Dependence on non-existing functionality!");
#define TODO(...)

#endif

#define detailed_assert(cond)                                                                    \
  if (!(cond))                                                                                   \
  {                                                                                              \
    std::cout << "Failed :" << #cond << " in " << __FILE__ << " line " << __LINE__ << std::endl; \
    throw std::runtime_error("Assertion failed");                                                \
  }
