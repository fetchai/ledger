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

#include "core/json/document.hpp"

#include "gtest/gtest.h"

using fetch::json::JSONDocument;
using fetch::variant::Variant;

TEST(JsonTests, SimpleParseTest)
{
  char const *text = R"({
    "empty": {},
    "array": [1,2,3,4,5],
    "arrayMixed": [
      {
        "value": 1
      },
      4
    ]
  })";

  // parse the JSON text
  JSONDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();

  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 3);
  EXPECT_TRUE(root.Has("empty"));
  EXPECT_TRUE(root.Has("array"));
  EXPECT_TRUE(root.Has("arrayMixed"));

  auto const &empty = doc["empty"];
  EXPECT_TRUE(empty.IsObject());
  EXPECT_EQ(empty.size(), 0);

  auto const &array = doc["array"];
  EXPECT_TRUE(array.IsArray());
  EXPECT_EQ(array.size(), 5);
  EXPECT_EQ(array[0].As<int>(), 1);
  EXPECT_EQ(array[1].As<int>(), 2);
  EXPECT_EQ(array[2].As<int>(), 3);
  EXPECT_EQ(array[3].As<int>(), 4);
  EXPECT_EQ(array[4].As<int>(), 5);

  auto const &array_mixed = doc["arrayMixed"];
  EXPECT_TRUE(array_mixed.IsArray());
  EXPECT_EQ(array_mixed.size(), 2);

  auto const &array_obj = array_mixed[0];
  EXPECT_TRUE(array_obj.IsObject());
  EXPECT_EQ(array_obj.size(), 1);
  EXPECT_TRUE(array_obj.Has("value"));
  EXPECT_EQ(array_obj["value"].As<int>(), 1);

  EXPECT_EQ(array_mixed[1].As<int>(), 4);
}

TEST(JsonTests, TypeParsing)
{
  char const *doc_content = R"({
  "a": 3,
  "b": 2.3e-2,
  "c": 2e+9,
  "d": "hello",
  "e": null,
  "f": true,
  "g": false
}
)";

  JSONDocument doc;
  doc.Parse(doc_content);
  EXPECT_EQ(doc["a"].type(), Variant::Type::INTEGER);
  EXPECT_EQ(doc["b"].type(), Variant::Type::FLOATING_POINT);
  EXPECT_EQ(doc["c"].type(), Variant::Type::FLOATING_POINT);
  EXPECT_EQ(doc["d"].type(), Variant::Type::STRING);
  EXPECT_EQ(doc["e"].type(), Variant::Type::NULL_VALUE);

  EXPECT_EQ(doc["f"].type(), Variant::Type::BOOLEAN);
  EXPECT_EQ(doc["g"].type(), Variant::Type::BOOLEAN);
}

TEST(JsonTests, ParsingExeptions)
{
  JSONDocument doc;
  EXPECT_THROW(doc.Parse("{"), fetch::json::JSONParseException);
  EXPECT_THROW(doc.Parse("{]"), fetch::json::JSONParseException);

  EXPECT_THROW(doc.Parse(R"(["a":"b"])"), fetch::json::JSONParseException);
  EXPECT_THROW(doc.Parse(R"({"a": 2.fs})"), fetch::json::JSONParseException);
}

TEST(JsonTests, LargeArray)
{
  static const std::size_t ARRAY_SIZE = 10000;

  std::string json_text;

  // formulate a large array of objects
  {
    std::ostringstream oss;

    oss << '[';

    for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
    {
      if (i)
      {
        oss << ',';
      }

      oss << R"({"value": )" << i << '}' << '\n';
    }

    oss << ']';

    // update the json text field
    json_text = oss.str();
  }

  // parse the JSON document
  JSONDocument doc;
  ASSERT_NO_THROW(doc.Parse(json_text));

  auto const &root = doc.root();

  ASSERT_TRUE(root.IsArray());
  ASSERT_EQ(root.size(), ARRAY_SIZE);

  for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
  {
    auto const &obj = root[i];

    ASSERT_TRUE(obj.IsObject());
    ASSERT_TRUE(obj.Has("value"));

    auto const &element = obj["value"];

    EXPECT_TRUE(element.Is<std::size_t>());
    EXPECT_EQ(element.As<std::size_t>(), i);
  }
}
