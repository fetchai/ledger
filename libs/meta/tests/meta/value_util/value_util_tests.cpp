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

#include "meta/slots.hpp"
#include "meta/value_util.hpp"

#include "gtest/gtest.h"

#include <sstream>
#include <string>

// googletest cannot into commas in assertions so we'll need few more macros defined.
#define ASSERT_HOLDS(...)               \
  {                                     \
    auto value = (__VA_ARGS__);         \
    ASSERT_TRUE(value) << #__VA_ARGS__; \
  }

#define ASSERT_UNTRUE(...) ASSERT_HOLDS(!(__VA_ARGS__))

// And since we're defining extra assertion macros why not add a few more specialized.
#define ASSERT_TYPE_EQ(...) ASSERT_HOLDS(std::is_same<__VA_ARGS__>::value)

TEST(ValueUtilTests, Slots)
{
  auto   f = fetch::value_util::Slots(fetch::value_util::Slot<int>([](auto x, auto y) {
                                      std::ostringstream s;
                                      s << x * y;
                                      return s.str();
                                    }),
                                    fetch::value_util::Slot<double>([](auto x, auto y) {
                                      std::ostringstream s;
                                      s << x + y;
                                      return s.str();
                                    }),
                                    [](auto &&x, auto &&y) { return std::string(x + y); });
  int    x = 3, y = 14;
  double a = 3.0, b = 0.14;
  ASSERT_HOLDS(f(x, y) == "42");
  ASSERT_HOLDS(f(a, b) == "3.14");
  ASSERT_HOLDS(f(std::string("Hi, "), "there!") == "Hi, there!");
}
