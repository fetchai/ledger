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

#include "storage/variant_stack.hpp"
#include "core/random/lfg.hpp"

#include "testing/unittest.hpp"

#include <iostream>
#include <stack>
using namespace fetch::storage;

#define TYPE uint64_t

int main()
{
  SCENARIO("usage of variant stack with basic type")
  {
    static constexpr int64_t                  N = 100;
    VariantStack                              stack;
    std::stack<TYPE>                          reference;
    fetch::random::LaggedFibonacciGenerator<> lfg;
    stack.New("variant_stack_test_1.db");

    SECTION_REF("populating the stack")
    {
      EXPECT(stack.empty());

      bool all_pass = true;
      for (int64_t i = 0; i < N; ++i)
      {

        SILENT_EXPECT((stack.size() == std::size_t(i)));
        all_pass &= (stack.size() == std::size_t(i));

        uint64_t val = lfg();
        reference.push(val);
        stack.Push(val);
        uint64_t ref;
        stack.Top(ref);

        SILENT_EXPECT(ref == val);
        all_pass &= (ref == val);
      }

      CHECK("populated correctly", all_pass);
    };

    SECTION_REF("checking that elements come out in the right order")
    {
      bool all_pass = true;
      for (std::size_t i = 0; i < N; ++i)
      {
        uint64_t top, ref;
        stack.Top(top);
        ref = reference.top();

        reference.pop();
        stack.Pop();

        SILENT_EXPECT(ref == top);
        all_pass &= (ref == top);
      }
      CHECK("all elements came out alright", all_pass);
      EXPECT(stack.empty());
    };
  };

  return 0;
}
