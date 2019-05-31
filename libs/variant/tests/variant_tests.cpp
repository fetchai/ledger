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
#include "variant/variant.hpp"

#include "gtest/gtest.h"

#include <stdexcept>
#include <string>

namespace {

class VariantTests : public ::testing::Test
{
protected:
  using Variant        = fetch::variant::Variant;
  using ConstByteArray = fetch::byte_array::ConstByteArray;
  using ByteArray      = fetch::byte_array::ByteArray;
};

TEST_F(VariantTests, PrimitiveConstruction)
{
  {
    Variant v;
    EXPECT_EQ(v.type(), Variant::Type::UNDEFINED);
  }

  {
    Variant v{1};
    EXPECT_EQ(v.type(), Variant::Type::INTEGER);
  }

  {
    Variant v{20u};
    EXPECT_EQ(v.type(), Variant::Type::INTEGER);
  }

  {
    Variant v{true};
    EXPECT_EQ(v.type(), Variant::Type::BOOLEAN);
  }

  {
    Variant v{3.14f};
    EXPECT_EQ(v.type(), Variant::Type::FLOATING_POINT);
  }

  {
    Variant v{2.71828};
    EXPECT_EQ(v.type(), Variant::Type::FLOATING_POINT);
  }
}

TEST_F(VariantTests, PrimitiveCopyConstruction)
{
  {
    Variant orig;
    Variant v{orig};
    EXPECT_EQ(v.type(), Variant::Type::UNDEFINED);
  }

  {
    Variant orig{1};
    Variant v{orig};
    EXPECT_EQ(v.type(), Variant::Type::INTEGER);
  }

  {
    Variant orig{20u};
    Variant v{orig};
    EXPECT_EQ(v.type(), Variant::Type::INTEGER);
  }

  {
    Variant orig{true};
    Variant v{orig};
    EXPECT_EQ(v.type(), Variant::Type::BOOLEAN);
  }

  {
    Variant orig{3.14f};
    Variant v{orig};
    EXPECT_EQ(v.type(), Variant::Type::FLOATING_POINT);
  }

  {
    Variant orig{2.71828};
    Variant v{orig};
    EXPECT_EQ(v.type(), Variant::Type::FLOATING_POINT);
  }
}

TEST_F(VariantTests, PrimitiveAssignment)
{
  Variant v;
  EXPECT_EQ(v.type(), Variant::Type::UNDEFINED);
  v = 1;
  EXPECT_EQ(v.type(), Variant::Type::INTEGER);
  v = 20u;
  EXPECT_EQ(v.type(), Variant::Type::INTEGER);
  v = false;
  EXPECT_EQ(v.type(), Variant::Type::BOOLEAN);
  v = 3.13f;
  EXPECT_EQ(v.type(), Variant::Type::FLOATING_POINT);
  v = 3.66;
  EXPECT_EQ(v.type(), Variant::Type::FLOATING_POINT);
}

TEST_F(VariantTests, CheckPrimitiveIsValue)
{
  {
    Variant v{1};

    EXPECT_FALSE(v.Is<bool>());
    EXPECT_TRUE(v.Is<unsigned int>());
    EXPECT_TRUE(v.Is<int>());
    EXPECT_FALSE(v.Is<float>());
    EXPECT_FALSE(v.Is<double>());
    EXPECT_FALSE(v.Is<std::string>());
    EXPECT_FALSE(v.Is<ConstByteArray>());
  }

  {
    Variant v{20u};

    EXPECT_FALSE(v.Is<bool>());
    EXPECT_TRUE(v.Is<unsigned int>());
    EXPECT_TRUE(v.Is<int>());
    EXPECT_FALSE(v.Is<float>());
    EXPECT_FALSE(v.Is<double>());
    EXPECT_FALSE(v.Is<std::string>());
    EXPECT_FALSE(v.Is<ConstByteArray>());
  }

  {
    Variant v{true};

    EXPECT_TRUE(v.Is<bool>());
    EXPECT_FALSE(v.Is<unsigned int>());
    EXPECT_FALSE(v.Is<int>());
    EXPECT_FALSE(v.Is<float>());
    EXPECT_FALSE(v.Is<double>());
    EXPECT_FALSE(v.Is<std::string>());
    EXPECT_FALSE(v.Is<ConstByteArray>());
  }

  {
    Variant v{3.14f};

    EXPECT_FALSE(v.Is<bool>());
    EXPECT_FALSE(v.Is<unsigned int>());
    EXPECT_FALSE(v.Is<int>());
    EXPECT_TRUE(v.Is<float>());
    EXPECT_TRUE(v.Is<double>());
    EXPECT_FALSE(v.Is<std::string>());
    EXPECT_FALSE(v.Is<ConstByteArray>());
  }

  {
    Variant v{2.71828};

    EXPECT_FALSE(v.Is<bool>());
    EXPECT_FALSE(v.Is<unsigned int>());
    EXPECT_FALSE(v.Is<int>());
    EXPECT_TRUE(v.Is<float>());
    EXPECT_TRUE(v.Is<double>());
    EXPECT_FALSE(v.Is<std::string>());
    EXPECT_FALSE(v.Is<ConstByteArray>());
  }
}

TEST_F(VariantTests, StringConstruction)
{
  {
    Variant v{"foobar"};

    EXPECT_EQ(v.As<ConstByteArray>(), "foobar");
  }

  {
    Variant v{ConstByteArray{"foobar"}};

    EXPECT_EQ(v.As<ConstByteArray>(), "foobar");
  }

  {
    Variant v{std::string{"foobar"}};

    EXPECT_EQ(v.As<ConstByteArray>(), "foobar");
  }
}

TEST_F(VariantTests, StringAssignment)
{
  Variant v;
  EXPECT_TRUE(v.IsUndefined());

  v = std::string{"foobar1"};
  EXPECT_TRUE(v.IsString());
  EXPECT_EQ(v.As<ConstByteArray>(), "foobar1");

  v = Variant::Undefined();
  EXPECT_TRUE(v.IsUndefined());

  v = ConstByteArray{"foobar2"};
  EXPECT_TRUE(v.IsString());
  EXPECT_EQ(v.As<ConstByteArray>(), "foobar2");

  v = Variant::Undefined();
  EXPECT_TRUE(v.IsUndefined());

  v = "foobar3";
  EXPECT_TRUE(v.IsString());
  EXPECT_EQ(v.As<ConstByteArray>(), "foobar3");
}

TEST_F(VariantTests, CheckNullAndUndefined)
{
  Variant v;
  EXPECT_TRUE(v.IsUndefined());

  v = Variant::Null();
  EXPECT_TRUE(v.IsNull());

  v = Variant::Undefined();
  EXPECT_TRUE(v.IsUndefined());
}

TEST_F(VariantTests, CheckElementAccess)
{
  {
    Variant v{1};

    EXPECT_TRUE(v.IsInteger());

    EXPECT_THROW(v.As<bool>(), std::runtime_error);
    EXPECT_EQ(1u, v.As<unsigned int>());
    EXPECT_EQ(1, v.As<int>());
    EXPECT_THROW(v.As<float>(), std::runtime_error);
    EXPECT_THROW(v.As<double>(), std::runtime_error);
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_THROW(v.As<ConstByteArray>(), std::runtime_error);
  }

  {
    Variant v{20u};

    EXPECT_TRUE(v.IsInteger());

    EXPECT_THROW(v.As<bool>(), std::runtime_error);
    EXPECT_EQ(20u, v.As<unsigned int>());
    EXPECT_EQ(20, v.As<int>());
    EXPECT_THROW(v.As<float>(), std::runtime_error);
    EXPECT_THROW(v.As<double>(), std::runtime_error);
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_THROW(v.As<ConstByteArray>(), std::runtime_error);
  }

  {
    Variant v{true};

    EXPECT_TRUE(v.IsBoolean());

    EXPECT_TRUE(v.As<bool>());
    EXPECT_THROW(v.As<unsigned int>(), std::runtime_error);
    EXPECT_THROW(v.As<int>(), std::runtime_error);
    EXPECT_THROW(v.As<float>(), std::runtime_error);
    EXPECT_THROW(v.As<double>(), std::runtime_error);
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_THROW(v.As<ConstByteArray>(), std::runtime_error);
  }

  {
    Variant v{3.14f};

    EXPECT_TRUE(v.IsFloatingPoint());

    EXPECT_THROW(v.As<bool>(), std::runtime_error);
    EXPECT_THROW(v.As<unsigned int>(), std::runtime_error);
    EXPECT_THROW(v.As<int>(), std::runtime_error);
    EXPECT_FLOAT_EQ(3.14f, v.As<float>());
    EXPECT_FLOAT_EQ(3.14f, static_cast<float>(v.As<double>()));
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_THROW(v.As<ConstByteArray>(), std::runtime_error);
  }

  {
    Variant v{2.71828};

    EXPECT_TRUE(v.IsFloatingPoint());

    EXPECT_THROW(v.As<bool>(), std::runtime_error);
    EXPECT_THROW(v.As<unsigned int>(), std::runtime_error);
    EXPECT_THROW(v.As<int>(), std::runtime_error);
    EXPECT_FLOAT_EQ(2.71828f, v.As<float>());
    EXPECT_DOUBLE_EQ(2.71828, v.As<double>());
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_THROW(v.As<ConstByteArray>(), std::runtime_error);
  }

  {
    Variant v{"this is a simple string"};

    EXPECT_TRUE(v.IsString());

    EXPECT_THROW(v.As<bool>(), std::runtime_error);
    EXPECT_THROW(v.As<unsigned int>(), std::runtime_error);
    EXPECT_THROW(v.As<int>(), std::runtime_error);
    EXPECT_THROW(v.As<float>(), std::runtime_error);
    EXPECT_THROW(v.As<double>(), std::runtime_error);
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_EQ(v.As<ConstByteArray>(), "this is a simple string");
  }

  {
    Variant v{std::string{"this is a simple string"}};

    EXPECT_TRUE(v.IsString());

    EXPECT_THROW(v.As<bool>(), std::runtime_error);
    EXPECT_THROW(v.As<unsigned int>(), std::runtime_error);
    EXPECT_THROW(v.As<int>(), std::runtime_error);
    EXPECT_THROW(v.As<float>(), std::runtime_error);
    EXPECT_THROW(v.As<double>(), std::runtime_error);
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_EQ(v.As<ConstByteArray>(), "this is a simple string");
  }

  {
    Variant v{ConstByteArray{"this is a simple string"}};

    EXPECT_TRUE(v.IsString());

    EXPECT_THROW(v.As<bool>(), std::runtime_error);
    EXPECT_THROW(v.As<unsigned int>(), std::runtime_error);
    EXPECT_THROW(v.As<int>(), std::runtime_error);
    EXPECT_THROW(v.As<float>(), std::runtime_error);
    EXPECT_THROW(v.As<double>(), std::runtime_error);
    // EXPECT_THROW(v.As<std::string>(), std::runtime_error);
    EXPECT_EQ(v.As<ConstByteArray>(), "this is a simple string");
  }
}

TEST_F(VariantTests, CheckCopyAssignment)
{
  {
    Variant v{1};
    Variant other;

    ASSERT_TRUE(v.IsInteger());
    ASSERT_TRUE(other.IsUndefined());

    other = v;

    ASSERT_TRUE(v.IsInteger());
    ASSERT_TRUE(other.IsInteger());

    EXPECT_EQ(1, v.As<int>());
  }

  {
    Variant v{20u};
    Variant other;

    ASSERT_TRUE(v.IsInteger());
    ASSERT_TRUE(other.IsUndefined());

    other = v;

    ASSERT_TRUE(v.IsInteger());
    ASSERT_TRUE(other.IsInteger());

    EXPECT_EQ(20, v.As<int>());
  }

  {
    Variant v{true};
    Variant other;

    ASSERT_TRUE(v.IsBoolean());
    ASSERT_TRUE(other.IsUndefined());

    other = v;

    ASSERT_TRUE(v.IsBoolean());
    ASSERT_TRUE(other.IsBoolean());

    EXPECT_EQ(true, v.As<bool>());
  }

  {
    Variant v{3.14f};
    Variant other;

    ASSERT_TRUE(v.IsFloatingPoint());
    ASSERT_TRUE(other.IsUndefined());

    other = v;

    ASSERT_TRUE(v.IsFloatingPoint());
    ASSERT_TRUE(other.IsFloatingPoint());

    EXPECT_FLOAT_EQ(3.14f, v.As<float>());
  }

  {
    Variant v{2.71828};
    Variant other;

    ASSERT_TRUE(v.IsFloatingPoint());
    ASSERT_TRUE(other.IsUndefined());

    other = v;

    ASSERT_TRUE(v.IsFloatingPoint());
    ASSERT_TRUE(other.IsFloatingPoint());

    EXPECT_DOUBLE_EQ(2.71828, v.As<double>());
  }

  {
    Variant v{"this is a simple string"};
    Variant other;

    ASSERT_TRUE(v.IsString());
    ASSERT_TRUE(other.IsUndefined());

    other = v;

    ASSERT_TRUE(v.IsString());
    ASSERT_TRUE(other.IsString());

    EXPECT_EQ(v.As<ConstByteArray>(), "this is a simple string");
  }

  {
    Variant obj                           = Variant::Object();
    obj["does"]                           = Variant::Object();
    obj["does"]["nested"]                 = Variant::Object();
    obj["does"]["nested"]["copy"]         = Variant::Object();
    obj["does"]["nested"]["copy"]["work"] = true;

    Variant next_obj{obj};

    EXPECT_TRUE(next_obj["does"]["nested"]["copy"]["work"].As<bool>());
  }
}

TEST_F(VariantTests, IndexBasedArrayInit)
{
  Variant v = Variant::Array(5);

  EXPECT_TRUE(v.IsArray());
  ASSERT_EQ(5, v.size());

  EXPECT_TRUE(v[0].IsUndefined());
  EXPECT_TRUE(v[1].IsUndefined());
  EXPECT_TRUE(v[2].IsUndefined());
  EXPECT_TRUE(v[3].IsUndefined());
  EXPECT_TRUE(v[4].IsUndefined());

  v[0] = true;
  v[1] = 1;
  v[2] = 3.14f;
  v[3] = 10e4;
  v[4] = "variant";

  EXPECT_TRUE(v[0].IsBoolean());
  EXPECT_TRUE(v[1].Is<int>());
  EXPECT_TRUE(v[2].Is<float>());
  EXPECT_TRUE(v[3].Is<double>());
  EXPECT_TRUE(v[4].IsString());
}

TEST_F(VariantTests, ListNesting)
{
  Variant v = Variant::Array(1);

  EXPECT_TRUE(v.IsArray());
  ASSERT_EQ(1, v.size());

  EXPECT_TRUE(v[0].IsUndefined());

  v[0] = Variant::Array(1);

  EXPECT_TRUE(v[0].IsArray());
  ASSERT_EQ(1, v[0].size());

  EXPECT_TRUE(v[0][0].IsUndefined());

  v[0][0] = Variant::Array(1);

  EXPECT_TRUE(v[0][0].IsArray());
  ASSERT_EQ(1, v[0][0].size());

  EXPECT_TRUE(v[0][0][0].IsUndefined());

  v[0][0][0] = "foobar";

  EXPECT_TRUE(v[0][0][0].IsString());
  EXPECT_EQ(v[0][0][0].As<ConstByteArray>(), "foobar");
  EXPECT_EQ(v[0][0][0].size(), 6);
}

TEST_F(VariantTests, BasicObject)
{
  Variant v         = Variant::Object();
  v["key"]          = "value";
  v["number"]       = 42;
  v["obj"]          = Variant::Object();
  v["obj"]["array"] = Variant::Array(1);
  v["obj"]["size"]  = 1;

  EXPECT_EQ(v.size(), 3);

  ASSERT_TRUE(v.Has("key"));
  EXPECT_TRUE(v["key"].IsString());
  EXPECT_EQ(v["key"].As<ConstByteArray>(), "value");

  ASSERT_TRUE(v.Has("number"));
  EXPECT_TRUE(v["number"].IsInteger());
  EXPECT_EQ(v["number"].As<int>(), 42);

  ASSERT_TRUE(v.Has("obj"));
  ASSERT_TRUE(v["obj"].IsObject());
  ASSERT_TRUE(v["obj"].Has("array"));
  EXPECT_TRUE(v["obj"]["array"].IsArray());
  EXPECT_EQ(v["obj"]["array"].size(), 1);
  EXPECT_TRUE(v["obj"]["array"][0].IsUndefined());

  ASSERT_TRUE(v["obj"].Has("size"));
  EXPECT_TRUE(v["obj"]["size"].IsInteger());
  EXPECT_EQ(v["obj"]["size"].As<int>(), 1);

  {
    Variant const &w(v);
    ASSERT_THROW(w["not-present"], std::out_of_range);
  }
}

TEST_F(VariantTests, SizeValues)
{
  Variant u;
  Variant i{10};
  Variant f{2.3};
  Variant s{"foo"};
  Variant n  = Variant::Null();
  Variant a  = Variant::Array(1);
  Variant o  = Variant::Object();
  o["key"]   = 2;
  o["value"] = 3;

  EXPECT_EQ(u.size(), 0);
  EXPECT_EQ(i.size(), 0);
  EXPECT_EQ(f.size(), 0);
  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(n.size(), 0);
  EXPECT_EQ(a.size(), 1);
  EXPECT_EQ(o.size(), 2);
}

TEST_F(VariantTests, ConstArrayAccess)
{
  Variant v = Variant::Array(1);
  v[0]      = 42;

  {
    Variant const &w{v};

    EXPECT_EQ(w[0].As<int>(), 42);
  }
}

TEST_F(VariantTests, ConstObjectAccess)
{
  Variant v = Variant::Object();
  v["key"]  = "value";

  {
    Variant const &w{v};

    ConstByteArray value = w["key"].As<ConstByteArray>();
    EXPECT_EQ(value, "value");
  }
}

TEST_F(VariantTests, IllAdvisedOperations)
{
  Variant a = Variant::Array(0);

  ASSERT_THROW(a.Has("key"), std::runtime_error);
  ASSERT_THROW(a["key"] = "value", std::runtime_error);

  Variant o = Variant::Object();
  ASSERT_THROW(o[5], std::runtime_error);

  {
    Variant const &ca{a};
    ASSERT_THROW(ca["key"], std::runtime_error);

    Variant const &co{o};
    ASSERT_THROW(co[5], std::runtime_error);
  }
}

}  // namespace
