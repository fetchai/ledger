//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/byte_array/tokenizer/tokenizer.hpp"
#include "core/byte_array/consumers.hpp"
#include "testing/unittest.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace fetch::byte_array;

bool equals(Tokenizer const &tokenizer, std::vector<fetch::byte_array::ConstByteArray> const &ref)
{
  if (ref.size() != tokenizer.size()) return false;
  for (std::size_t i = 0; i < ref.size(); ++i)
  {
    if (ref[i] != tokenizer[i]) return false;
  }

  return true;
}

bool equals(Tokenizer const &tokenizer, std::vector<int> const &ref)
{
  if (ref.size() != tokenizer.size()) return false;
  for (std::size_t i = 0; i < ref.size(); ++i)
  {
    if (ref[i] != tokenizer[i].type()) return false;
  }

  return true;
}

int main(int argc, char **argv)
{
  enum
  {
    E_INTEGER        = 0,
    E_FLOATING_POINT = 1,
    E_STRING         = 2,
    E_KEYWORD        = 3,
    E_TOKEN          = 4,
    E_WHITESPACE     = 5,
    E_CATCH_ALL      = 6
  };

  SCENARIO("Testing individual consumers")
  {
    SECTION("Any character")
    {
      Tokenizer test;
      test.AddConsumer(fetch::byte_array::consumers::AnyChar<E_CATCH_ALL>);
      std::string test_str;

      test_str = "hello world";
      EXPECT(test.Parse(test_str));
      EXPECT(test.size() == test_str.size());

      test_str = "12$1adf)(SD)S(*ASdf 09812 4e12";
      EXPECT(test.Parse(test_str));
      EXPECT(test.size() == test_str.size());

      test_str =
          "12$1adf)(SD)S(*ASdf 09812 4e12asd kalhsdak shd aopisfu q[wr "
          "iqrw'prkas'd;fkla;s'dfl;ak \"";
      EXPECT(test.Parse(test_str));
      EXPECT(test.size() == test_str.size());
    };

    SECTION("Any character")
    {
      Tokenizer test;
      test.AddConsumer(fetch::byte_array::consumers::NumberConsumer<E_INTEGER, E_FLOATING_POINT>);
      test.AddConsumer(fetch::byte_array::consumers::AnyChar<E_CATCH_ALL>);

      std::string test_str;

      test_str = "93 -12.31 -12.e+3";
      EXPECT(test.Parse(test_str));
      EXPECT(equals(test, {"93", " ", "-12.31", " ", "-12.e+3"}));
      EXPECT(
          equals(test, {E_INTEGER, E_CATCH_ALL, E_FLOATING_POINT, E_CATCH_ALL, E_FLOATING_POINT}));
    };
  };

  // TODO(issue 37): Add more tests
}
