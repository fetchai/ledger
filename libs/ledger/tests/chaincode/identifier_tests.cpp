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

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/byte_array/encoders.hpp"
#include "ledger/identifier.hpp"
#include "ledger/chain/v2/address.hpp"

#include <gtest/gtest.h>

using fetch::ledger::Identifier;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;
using fetch::byte_array::ToBase64;
using fetch::ledger::v2::Address;

ConstByteArray GenerateSequence(std::size_t size)
{
  ByteArray buffer{};
  buffer.Resize(size);

  for (std::size_t i = 0; i < size; ++i)
  {
    buffer[i] = i & 0xff;
  }

  return {buffer};
}

TEST(IdentifierTests, BasicChecks)
{
  Identifier id("foo.bar.baz");
  EXPECT_EQ(id.name(), "baz");
  EXPECT_EQ(id.name_space(), "foo.bar");
  EXPECT_EQ(id[0], "foo");
  EXPECT_EQ(id[1], "bar");
  EXPECT_EQ(id[2], "baz");
  EXPECT_EQ(Identifier::Type::NORMAL, id.type());
  EXPECT_EQ(ConstByteArray{"foo.bar.baz"}, id.qualifier());
}

TEST(IdentifierTests, DirectParent)
{
  Identifier parent("foo");
  Identifier child("foo.bar");
  EXPECT_TRUE(parent.IsParentTo(child));
  EXPECT_TRUE(child.IsChildTo(parent));
  EXPECT_TRUE(parent.IsDirectParentTo(child));
  EXPECT_TRUE(child.IsDirectChildTo(parent));
  EXPECT_FALSE(parent.IsChildTo(child));
  EXPECT_FALSE(child.IsParentTo(parent));
  EXPECT_EQ(Identifier::Type::NORMAL, parent.type());
  EXPECT_EQ(Identifier::Type::NORMAL, child.type());
}

TEST(IdentifierTests, InDirectParent)
{
  Identifier parent("foo");
  Identifier child("foo.bar.baz");
  EXPECT_TRUE(parent.IsParentTo(child));
  EXPECT_TRUE(child.IsChildTo(parent));
  EXPECT_FALSE(parent.IsDirectParentTo(child));
  EXPECT_FALSE(child.IsDirectChildTo(parent));
  EXPECT_FALSE(parent.IsChildTo(child));
  EXPECT_FALSE(child.IsParentTo(parent));
  EXPECT_EQ(Identifier::Type::NORMAL, parent.type());
  EXPECT_EQ(Identifier::Type::NORMAL, child.type());
}

TEST(IdentifierTests, Child)
{
  Identifier parent("foo.baz");
  Identifier child("foo.bar");
  EXPECT_FALSE(parent.IsParentTo(child));
  EXPECT_FALSE(child.IsChildTo(parent));
  EXPECT_FALSE(child.IsParentTo(parent));
  EXPECT_FALSE(parent.IsChildTo(child));
  EXPECT_EQ(Identifier::Type::NORMAL, parent.type());
  EXPECT_EQ(Identifier::Type::NORMAL, child.type());
}

TEST(IdentifierTests, CheckInvalid)
{
  Identifier id{};
  EXPECT_FALSE(id.Parse("foo..baz"));

  // check that the exception is thrown from the constructor also
  ASSERT_THROW(Identifier{"foo..baz"}, std::runtime_error);
}

TEST(IdentifierTests, CheckSmartContractDigest)
{
  auto const digest = GenerateSequence(32);

  Identifier id{ToHex(digest)};

  EXPECT_EQ(Identifier::Type::SMART_CONTRACT, id.type());
  EXPECT_EQ(ToHex(digest), id.qualifier());
}

TEST(IdentifierTests, CheckContractName)
{
  auto const digest     = GenerateSequence(32);
  auto const public_key = GenerateSequence(32);

  Address address{public_key};

  auto const ns = ToHex(digest) + "." + address.display();

  Identifier id{ns + "." + "main"};

  EXPECT_EQ(Identifier::Type::SMART_CONTRACT, id.type());
  EXPECT_EQ(ns, id.name_space());
  EXPECT_EQ(ConstByteArray{"main"}, id.name());
  EXPECT_EQ(ns + "." + "main", id.full_name());
  EXPECT_EQ(ToHex(digest), id.qualifier());
}
