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

#include "ledger/identifier.hpp"
#include "testing/unittest.hpp"

using fetch::ledger::Identifier;

int main()
{

  SCENARIO("Parsing")
  {
    SECTION("Basic Checks")
    {
      Identifier id("foo.bar.baz");

      EXPECT(id.name() == "baz");
      EXPECT(id.name_space() == "foo.bar");
      EXPECT(id[0] == "foo");
      EXPECT(id[1] == "bar");
      EXPECT(id[2] == "baz");
    };
  };

  SCENARIO("Ancestry Checks")
  {
    SECTION("Direct Parent")
    {
      Identifier parent("foo");
      Identifier child("foo.bar");

      EXPECT(parent.IsParentTo(child));
      EXPECT(child.IsChildTo(parent));
      EXPECT(parent.IsDirectParentTo(child));
      EXPECT(child.IsDirectChildTo(parent));
      EXPECT(!parent.IsChildTo(child));
      EXPECT(!child.IsParentTo(parent));
    };

    SECTION("Indirect Parent")
    {
      Identifier parent("foo");
      Identifier child("foo.bar.baz");

      EXPECT(parent.IsParentTo(child));
      EXPECT(child.IsChildTo(parent));
      EXPECT(!parent.IsDirectParentTo(child));
      EXPECT(!child.IsDirectChildTo(parent));
      EXPECT(!parent.IsChildTo(child));
      EXPECT(!child.IsParentTo(parent));
    };

    SECTION("Child")
    {
      Identifier parent("foo.baz");
      Identifier child("foo.bar");

      EXPECT(!parent.IsParentTo(child));
      EXPECT(!child.IsChildTo(parent));
      EXPECT(!child.IsParentTo(parent));
      EXPECT(!parent.IsChildTo(child));
    };
  };

  return 0;
}