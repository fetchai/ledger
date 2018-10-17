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

#include "core/json/document.hpp"
#include <iostream>
#include <gtest/gtest.h>
using namespace fetch::json;
using namespace fetch::byte_array;


TEST(json_bsic_parsing_gtest, Nathan_test)
{
  ByteArray    data = "{\"HTTPPort\": 8081, \"IP\": \"localhost\", \"TCPPort\": 9081}";
  JSONDocument test(data);
  std::cout << data << std::endl;
  std::cout << "---" << std::endl;
  std::cout << test.root() << std::endl;

  std::string IP_   = std::string(test["IP"].as_byte_array());
  uint32_t    port_ = test["TCPPort"].As<uint16_t>();

  std::cerr << "port is is " << port_ << std::endl;
  std::cerr << "IP is " << IP_ << " >>> " << test["IP"] << std::endl;
}

TEST(json_bsic_parsing_gtest, Parsing_and_modification_of_document)
{
  ByteArray doc_content = R"({
  "a": 3,
  "x": { 
    "y": [1,2,3],
    "z": null,
    "q": [],
    "hello world": {}
  }
}
)";

  JSONDocument doc;
  doc.Parse(doc_content);

  std::stringstream ss;
  ss << doc.root();
  EXPECT_EQ(ss.str(),
            R"({"a": 3, "x": {"y": [1, 2, 3], "z": null, "q": [], "hello world": {}}})");

  doc["a"] = 4;
  ss.str("");
  ss << doc.root();
  EXPECT_EQ(ss.str(),
            R"({"a": 4, "x": {"y": [1, 2, 3], "z": null, "q": [], "hello world": {}}})");

  doc["x"]["y"][1] = 5;
  ss.str("");
  ss << doc.root();
  EXPECT_EQ(ss.str(),
            R"({"a": 4, "x": {"y": [1, 5, 3], "z": null, "q": [], "hello world": {}}})");

  doc["x"]["z"] = fetch::script::Variant({1, 2, 3, 4, 5});
  ss.str("");
  ss << doc.root();
  EXPECT_EQ(ss.str(),
            R"({"a": 4, "x": {"y": [1, 5, 3], "z": [1, 2, 3, 4, 5], "q": [], "hello world": {}}})");

  ss.str("");
  ss << doc["x"]["y"];
  EXPECT_EQ("[1, 5, 3]", ss.str());
}

TEST(json_bsic_parsing_gtest, Type_parsing)
{
  ByteArray doc_content = R"({
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
  EXPECT_EQ(doc["a"].type(), fetch::script::VariantType::INTEGER);
  EXPECT_EQ(doc["b"].type(), fetch::script::VariantType::FLOATING_POINT);
  EXPECT_EQ(doc["c"].type(), fetch::script::VariantType::FLOATING_POINT);
  EXPECT_EQ(doc["d"].type(), fetch::script::VariantType::STRING);
  EXPECT_EQ(doc["e"].type(), fetch::script::VariantType::NULL_VALUE);

  EXPECT_EQ(doc["f"].type(), fetch::script::VariantType::BOOLEAN);
  EXPECT_EQ(doc["g"].type(), fetch::script::VariantType::BOOLEAN);
}

TEST(json_bsic_parsing_gtest, Parsing_exeptions)
{
  JSONDocument doc;
  EXPECT_THROW(doc.Parse("{"), fetch::json::JSONParseException);
  EXPECT_THROW(doc.Parse("{]"), fetch::json::JSONParseException);

  EXPECT_THROW(doc.Parse(R"(["a":"b"])"), fetch::json::JSONParseException);
  EXPECT_THROW(doc.Parse(R"({"a": 2.fs})"), fetch::json::JSONParseException);
}