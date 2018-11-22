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

#include "ledger/identifier.hpp"
#include <gtest/gtest.h>

 using fetch::ledger::Identifier;

 TEST(identifier_gtest, basic_checks)
{
  Identifier id("foo.bar.baz");
  EXPECT_EQ(id.name(), "baz");
  EXPECT_EQ(id.name_space(), "foo.bar");
  EXPECT_EQ(id[0], "foo");
  EXPECT_EQ(id[1], "bar");
  EXPECT_EQ(id[2], "baz");
}
 TEST(identifier_ancestry_checks, direct_parent)
{
  Identifier parent("foo");
  Identifier child("foo.bar");
  EXPECT_TRUE(parent.IsParentTo(child));
  EXPECT_TRUE(child.IsChildTo(parent));
  EXPECT_TRUE(parent.IsDirectParentTo(child));
  EXPECT_TRUE(child.IsDirectChildTo(parent));
  EXPECT_FALSE(parent.IsChildTo(child));
  EXPECT_FALSE(child.IsParentTo(parent));
}
 TEST(identifier_ancestry_checks, indirect_Parent)
{
  Identifier parent("foo");
  Identifier child("foo.bar.baz");
  EXPECT_TRUE(parent.IsParentTo(child));
  EXPECT_TRUE(child.IsChildTo(parent));
  EXPECT_FALSE(parent.IsDirectParentTo(child));
  EXPECT_FALSE(child.IsDirectChildTo(parent));
  EXPECT_FALSE(parent.IsChildTo(child));
  EXPECT_FALSE(child.IsParentTo(parent));
}
 TEST(identifier_ancestry_checks, Child)
{
  Identifier parent("foo.baz");
  Identifier child("foo.bar");
  EXPECT_FALSE(parent.IsParentTo(child));
  EXPECT_FALSE(child.IsChildTo(parent));
  EXPECT_FALSE(child.IsParentTo(parent));
  EXPECT_FALSE(parent.IsChildTo(child));
}
 TEST(identifier_ancestry_checks, Append)
{
  Identifier id;
  id.Append("foo");
  id.Append("bar");
  id.Append("baz");
  id.Append("x.y.z");
  EXPECT_EQ(id.full_name(), "foo.bar.baz.x.y.z");
}
 TEST(identifier_ancestry_checks, Append_invalid_namespace_at_beginning)
{
  Identifier id;
   bool exception_received = false;
   try
  {
    id.Append(".foo");
  }
  catch (std::runtime_error const &ex)
  {
    exception_received = true;
  }
  EXPECT_TRUE(exception_received);
}
 TEST(identifier_ancestry_checks, Append_invalid_namespace_in_the_middle)
{
  Identifier id;
  id.Append("foo");
   bool exception_received = false;
   try
  {
    id.Append(".bar");
  }
  catch (std::runtime_error const &ex)
  {
    exception_received = true;
  }
  EXPECT_TRUE(exception_received);
}