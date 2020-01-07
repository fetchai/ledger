//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "yaml/document.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <string>

using fetch::yaml::YamlDocument;
using string = std::string;

TEST(YamlTests, SimpleMappingTest)
{
  char const *text = "one: two\nthree: four";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 2);
  EXPECT_TRUE(root.Has("one"));
  EXPECT_TRUE(root.Has("three"));

  auto const &one = root["one"];
  EXPECT_EQ(one.As<string>(), "two");

  auto const &three = root["three"];
  EXPECT_EQ(three.As<string>(), "four");
}

TEST(YamlTests, MultiLevelMappingTest)
{
  char const *text = R"(- key: level1
  child:
    - key: level2
      child:
        - key: level3)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsArray());
  EXPECT_EQ(root.size(), 1);

  auto const &l0item = root[0];
  EXPECT_TRUE(l0item.IsObject());
  EXPECT_EQ(l0item.size(), 2);

  EXPECT_TRUE(l0item.Has("key"));
  EXPECT_TRUE(l0item.Has("child"));

  auto const &l1key = l0item["key"];
  EXPECT_EQ(l1key.As<string>(), "level1");

  auto const &l1child = l0item["child"];
  EXPECT_TRUE(l1child.IsArray());
  EXPECT_EQ(l1child.size(), 1);

  auto const &l1item = l1child[0];

  EXPECT_TRUE(l1item.IsObject());
  EXPECT_EQ(l1item.size(), 2);
  EXPECT_TRUE(l1item.Has("key"));
  EXPECT_TRUE(l1item.Has("child"));

  auto const &l2key = l1item["key"];
  EXPECT_EQ(l2key.As<string>(), "level2");
  auto const &l2child = l1item["child"];
  EXPECT_TRUE(l2child.IsArray());
  EXPECT_EQ(l2child.size(), 1);

  auto const &l2item = l2child[0];

  EXPECT_TRUE(l2item.IsObject());
  EXPECT_EQ(l2item.size(), 1);
  EXPECT_TRUE(l2item.Has("key"));

  auto const &l3key = l2item["key"];
  EXPECT_EQ(l3key.As<string>(), "level3");
}

TEST(YamlTests, Example2_2Test)
{  // Example 2.2
  char const *text = R"(hr:   65   # Home runs
avg: 0.278   # Batting average
rbi: 147     # Runs Batted In)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 3);
  EXPECT_TRUE(root.Has("hr"));
  EXPECT_TRUE(root.Has("avg"));
  EXPECT_TRUE(root.Has("rbi"));

  auto const &hr = root["hr"];
  EXPECT_EQ(hr.As<int>(), 65);
  auto const &rbi = root["rbi"];
  EXPECT_EQ(rbi.As<int>(), 147);
}

TEST(YamlTests, Example2_3Test)
{  // Example 2.3
  char const *text = R"(american:
  - Boston Red Sox
  - Detroit Tigers
  - New York Yankees
national:
  - New York Mets
  - Chicago Cubs
  - Atlanta Braves)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 2);
  EXPECT_TRUE(root.Has("american"));
  EXPECT_TRUE(root.Has("national"));

  auto const &american = root["american"];
  EXPECT_TRUE(american.IsArray());
  EXPECT_EQ(american.size(), 3);

  auto const &national = root["national"];
  EXPECT_TRUE(national.IsArray());
  EXPECT_EQ(national.size(), 3);
}

TEST(YamlTests, Example2_4Test)
{  // Example 2.4
  char const *text = R"(-
  name: Mark McGwire
  hr:   65
  avg:  0.278
-
  name: Sammy Sosa
  hr:   63
  avg:  0.288)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsArray());
  EXPECT_EQ(root.size(), 2);

  auto const &obj0 = root[0];
  EXPECT_TRUE(obj0.IsObject());
  EXPECT_EQ(obj0.size(), 3);
  EXPECT_TRUE(obj0.Has("name"));
  EXPECT_TRUE(obj0.Has("hr"));
  EXPECT_TRUE(obj0.Has("avg"));
  auto const &name0 = obj0["name"];
  EXPECT_EQ(name0.As<string>(), "Mark McGwire");
  auto const &hr0 = obj0["hr"];
  EXPECT_EQ(hr0.As<int>(), 65);

  auto const &obj1 = root[1];
  EXPECT_TRUE(obj1.IsObject());
  EXPECT_EQ(obj1.size(), 3);
  EXPECT_TRUE(obj1.Has("name"));
  EXPECT_TRUE(obj1.Has("hr"));
  EXPECT_TRUE(obj1.Has("avg"));

  auto const &name1 = obj1["name"];
  EXPECT_EQ(name1.As<string>(), "Sammy Sosa");
  auto const &hr1 = obj1["hr"];
  EXPECT_EQ(hr1.As<int>(), 63);
}

TEST(YamlTests, Example2_6Test)
{  // Example 2.6. Mapping of Mappings
  char const *text = R"(Mark McGwire: {hr: 65, avg: 0.278}
Sammy Sosa: {
    hr: 63,
    avg: 0.288
  })";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 2);
  EXPECT_TRUE(root.Has("Mark McGwire"));
  EXPECT_TRUE(root.Has("Sammy Sosa"));

  auto const &obj0 = root["Mark McGwire"];
  EXPECT_TRUE(obj0.IsObject());
  EXPECT_EQ(obj0.size(), 2);
  EXPECT_TRUE(obj0.Has("hr"));
  EXPECT_TRUE(obj0.Has("avg"));
  auto const &hr0 = obj0["hr"];
  EXPECT_EQ(hr0.As<int>(), 65);

  auto const &obj1 = root["Sammy Sosa"];
  EXPECT_TRUE(obj1.IsObject());
  EXPECT_EQ(obj1.size(), 2);
  EXPECT_TRUE(obj1.Has("hr"));
  EXPECT_TRUE(obj1.Has("avg"));
  auto const &hr1 = obj1["hr"];
  EXPECT_EQ(hr1.As<int>(), 63);
}

TEST(YamlTests, Example2_9Test)
{  // Example 2.9  Single Document with Two Comments
  char const *text = R"(---
hr: # 1998 hr ranking
  - Mark McGwire
  - Sammy Sosa
rbi:
  # 1998 rbi ranking
  - Sammy Sosa
  - Ken Griffey)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 2);
  EXPECT_TRUE(root.Has("hr"));
  EXPECT_TRUE(root.Has("rbi"));

  auto const &obj0 = root["hr"];
  EXPECT_TRUE(obj0.IsArray());
  EXPECT_EQ(obj0.size(), 2);

  auto const &obj1 = root["rbi"];
  EXPECT_TRUE(obj1.IsArray());
  EXPECT_EQ(obj1.size(), 2);
}

TEST(YamlTests, Example2_10Test)
{  // Example 2.10.  Node for “Sammy Sosa” appears twice in this document
  char const *text = R"(---
hr:
  - Mark McGwire
  # Following node labeled SS
  - &SS Sammy Sosa
rbi:
  - *SS # Subsequent occurrence
  - Ken Griffey)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 2);
  EXPECT_TRUE(root.Has("hr"));
  EXPECT_TRUE(root.Has("rbi"));

  auto const &obj0 = root["hr"];
  EXPECT_TRUE(obj0.IsArray());
  EXPECT_EQ(obj0.size(), 2);
  auto const &name0 = obj0[1];
  EXPECT_EQ(name0.As<string>(), "Sammy Sosa");

  auto const &obj1 = root["rbi"];
  EXPECT_TRUE(obj1.IsArray());
  EXPECT_EQ(obj1.size(), 2);
  auto const &name1 = obj1[0];
  EXPECT_EQ(name1.As<string>(), "Sammy Sosa");
}

TEST(YamlTests, Example2_12Test)
{  // Example 2.12 Compact Nested Mapping
  char const *text = R"(# Products purchased
- item    : Super Hoop
  quantity: 1
- item    : Basketball
  quantity: 4
- item    : Big Shoes
  quantity: 1
)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsArray());
  EXPECT_EQ(root.size(), 3);

  auto const &obj0 = root[0];
  EXPECT_TRUE(obj0.IsObject());
  EXPECT_EQ(obj0.size(), 2);
  auto const &name0 = obj0["item"];
  EXPECT_EQ(name0.As<string>(), "Super Hoop");
  auto const &quantity0 = obj0["quantity"];
  EXPECT_EQ(quantity0.As<int>(), 1);

  auto const &obj1 = root[1];
  EXPECT_TRUE(obj1.IsObject());
  EXPECT_EQ(obj1.size(), 2);
  auto const &name1 = obj1["item"];
  EXPECT_EQ(name1.As<string>(), "Basketball");
  auto const &quantity1 = obj1["quantity"];
  EXPECT_EQ(quantity1.As<int>(), 4);

  auto const &obj2 = root[2];
  EXPECT_TRUE(obj2.IsObject());
  EXPECT_EQ(obj2.size(), 2);
  auto const &name2 = obj2["item"];
  EXPECT_EQ(name2.As<string>(), "Big Shoes");
  auto const &quantity2 = obj2["quantity"];
  EXPECT_EQ(quantity2.As<int>(), 1);
}

TEST(YamlTests, Example2_16Test)
{  // Example 2.16.  Indentation determines scope
  char const *text = R"(name: Mark McGwire
accomplishment: >
  Mark set a major league
  home run record in 1998.
stats: |
  65 Home Runs
  0.278 Batting Average)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 3);
  EXPECT_TRUE(root.Has("name"));
  EXPECT_TRUE(root.Has("accomplishment"));
  EXPECT_TRUE(root.Has("stats"));

  auto const &name = root["name"];
  EXPECT_EQ(name.As<string>(), "Mark McGwire");

  auto const &acc = root["accomplishment"];
  EXPECT_EQ(acc.As<string>(), "Mark set a major league home run record in 1998.");

  auto const &stats = root["stats"];
  EXPECT_EQ(stats.As<string>(), "65 Home Runs\n0.278 Batting Average");
}

TEST(YamlTests, Example2_17Test)
{  // Example 2.17. Quoted Scalars
  char const *text =
      "unicode: Sosa did fine.\\u263A\r\n"
      "control: \\b1998\\t1999\\t2000\\n\r\n"
      "hex esc: \\x0d\\x0a is \\r\\n\r\n"
      "single: '\"Howdy!\" he cried.'\r\n"
      "quoted: ' # Not a ''comment''.'\r\n"
      "tie-fighter: '|\\-*-/|'";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 6);
  EXPECT_TRUE(root.Has("unicode"));
  EXPECT_TRUE(root.Has("control"));
  EXPECT_TRUE(root.Has("hex esc"));
  EXPECT_TRUE(root.Has("single"));
  EXPECT_TRUE(root.Has("quoted"));
  EXPECT_TRUE(root.Has("tie-fighter"));

  auto const &unicode = root["unicode"];
  EXPECT_EQ(unicode.As<string>(), "Sosa did fine.&:");
  auto const &control = root["control"];
  EXPECT_EQ(control.As<string>(), "\b1998\t1999\t2000\n");
  auto const &hex = root["hex esc"];
  EXPECT_EQ(hex.As<string>(), "\r\n is \r\n");
  auto const &single = root["single"];
  EXPECT_EQ(single.As<string>(), "\"Howdy!\" he cried.");
  auto const &quoted = root["quoted"];
  EXPECT_EQ(quoted.As<string>(), " # Not a ''comment''.");
  auto const &tie = root["tie-fighter"];
  EXPECT_EQ(tie.As<string>(), "|\\-*-/|");
}

TEST(YamlTests, Example2_18Test)
{  // Example 2.18. Multi-line Flow Scalars
  char const *text = R"(plain:
  This unquoted scalar
  spans many lines.

quoted: "So does this
  quoted scalar.")";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 2);
  EXPECT_TRUE(root.Has("plain"));
  EXPECT_TRUE(root.Has("quoted"));

  auto const &plain = root["plain"];
  EXPECT_EQ(plain.As<string>(), "This unquoted scalar spans many lines.");

  auto const &quoted = root["quoted"];
  EXPECT_EQ(quoted.As<string>(), "So does this quoted scalar.");
}

TEST(YamlTests, Example2_19Test)
{  // Example 2.19. Integers
  char const *text = R"(canonical: 12345
decimal: +12345
octal: 0o14
hexadecimal: 0xC)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 4);
  EXPECT_TRUE(root.Has("canonical"));
  EXPECT_TRUE(root.Has("decimal"));
  EXPECT_TRUE(root.Has("octal"));
  EXPECT_TRUE(root.Has("hexadecimal"));

  auto const &canonical = root["canonical"];
  EXPECT_EQ(canonical.As<int>(), 12345);
  auto const &decimal = root["decimal"];
  EXPECT_EQ(decimal.As<int>(), 12345);
  auto const &octal = root["octal"];
  EXPECT_EQ(octal.As<int>(), 12);
  auto const &hexadecimal = root["hexadecimal"];
  EXPECT_EQ(hexadecimal.As<int>(), 12);
}

TEST(YamlTests, Example2_20Test)
{  // Example 2.20. Floating Point
  char const *text = R"(canonical: 1.23015e+3
exponential: 12.3015e+02
fixed: 1230.15
negative infinity: -.inf
not a number: .NaN)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 5);
  EXPECT_TRUE(root.Has("canonical"));
  EXPECT_TRUE(root.Has("exponential"));
  EXPECT_TRUE(root.Has("fixed"));
  EXPECT_TRUE(root.Has("negative infinity"));
  EXPECT_TRUE(root.Has("not a number"));

  auto const &canonical = root["canonical"];
  EXPECT_DOUBLE_EQ(1230.15, canonical.As<double>());
  auto const &exponential = root["exponential"];
  EXPECT_DOUBLE_EQ(1230.15, exponential.As<double>());
  auto const &fixed = root["fixed"];
  EXPECT_DOUBLE_EQ(1230.15, fixed.As<double>());
  auto const &inf = root["negative infinity"];
  EXPECT_TRUE(std::isinf(inf.As<double>()));
  auto const &nan = root["not a number"];
  EXPECT_TRUE(std::isnan(nan.As<double>()));
}

TEST(YamlTests, Example2_21Test)
{  // Example 2.21. Miscellaneous
  char const *text = R"(null:
booleans: [ true, false ]
string: '012345')";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 3);
  EXPECT_TRUE(root.Has("null"));
  EXPECT_TRUE(root.Has("booleans"));
  EXPECT_TRUE(root.Has("string"));

  auto const &null = root["null"];
  EXPECT_TRUE(null.IsNull());

  auto const &booleans = root["booleans"];
  EXPECT_TRUE(booleans.IsArray());
  EXPECT_EQ(booleans.size(), 2);
  EXPECT_TRUE(booleans[0].As<bool>());
  EXPECT_FALSE(booleans[1].As<bool>());
  auto const &str = root["string"];
  EXPECT_EQ(str.As<string>(), "012345");
}

TEST(YamlTests, Example2_22Test)
{  // Example 2.22. Timestamps
  char const *text = R"(canonical: 2001-12-15T02:59:43.1Z
iso8601: 2001-12-14t21:59:43.10-05:00
spaced: 2001-12-14 21:59:43.10 -5
date: 2002-12-14)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 4);
  EXPECT_TRUE(root.Has("canonical"));
  EXPECT_TRUE(root.Has("iso8601"));
  EXPECT_TRUE(root.Has("spaced"));
  EXPECT_TRUE(root.Has("date"));

  auto const &canonical = root["canonical"];
  EXPECT_EQ(canonical.As<string>(), "2001-12-15T02:59:43.1Z");
  auto const &iso8601 = root["iso8601"];
  EXPECT_EQ(iso8601.As<string>(), "2001-12-14t21:59:43.10-05:00");
  auto const &spaced = root["spaced"];
  EXPECT_EQ(spaced.As<string>(), "2001-12-14 21:59:43.10 -5");
  auto const &date = root["date"];
  EXPECT_EQ(date.As<string>(), "2002-12-14");
}

TEST(YamlTests, Example2_23Test)
{  // Example 2.23. Various Explicit Tags
  char const *text = R"(---
not-date: !!str 2002-04-28

picture: !!binary |
 R0lGODlhDAAMAIQAAP//9/X
 17unp5WZmZgAAAOfn515eXv
 Pz7Y6OjuDg4J+fn5OTk6enp
 56enmleECcgggoBADs=

application specific tag: !something |
 The semantics of the tag
 above may be different for
 different documents.)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 3);
  EXPECT_TRUE(root.Has("not-date"));
  EXPECT_TRUE(root.Has("picture"));
  EXPECT_TRUE(root.Has("application specific tag"));

  auto const &notdate = root["not-date"];
  EXPECT_EQ(notdate.As<string>(), "2002-04-28");
  auto const &picture = root["picture"];
  EXPECT_EQ(picture.As<string>(),
            "R0lGODlhDAAMAIQAAP//9/"
            "X\n17unp5WZmZgAAAOfn515eXv\nPz7Y6OjuDg4J+fn5OTk6enp\n56enmleECcgggoBADs=");
  auto const &spectag = root["application specific tag"];
  EXPECT_EQ(spectag.As<string>(),
            "The semantics of the tag\nabove may be different for\ndifferent documents.");
}

TEST(YamlTests, Example2_24Test)
{  // Example 2.24. Global Tags
  char const *text = R"(
--- !shape
  # Use the ! handle for presenting
  # tag:clarkevans.com,2002:circle
- !circle
  center: &ORIGIN {x: 73, y: 129}
  radius: 7
- !line
  start: *ORIGIN
  finish: { x: 89, y: 102 }
- !label
  start: *ORIGIN
  color: 0xFFEEBB
  text: Pretty vector drawing.)";

  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();
  EXPECT_TRUE(root.IsArray());
  EXPECT_EQ(root.size(), 3);

  auto const &circle = root[0];
  auto const &line   = root[1];
  auto const &label  = root[2];

  EXPECT_TRUE(circle.IsObject());
  EXPECT_TRUE(line.IsObject());
  EXPECT_TRUE(label.IsObject());
}

TEST(YamlTests, FullDocumentTest)
{
  char const *text = R"(--- !<tag:clarkevans.com,2002:invoice>
invoice: 34843
date   : 2001-01-23
bill-to: &id001
    given  : Chris
    family : Dumars
    address:
        lines: |
            458 Walkman Dr.
            Suite #292
        city    : Royal Oak
        state   : MI
        postal  : 48046
ship-to: *id001
product:
    - sku         : BL394D
      quantity    : 4
      description : Basketball
      price       : 450.00
    - sku         : BL4438H
      quantity    : 1
      description : Super Hoop
      price       : 2392.00
tax  : 251.42
total: 4443.52
comments:
    Late afternoon is best.
    Backup contact is Nancy
    Billsmer @ 338-4338.
)";

  // parse the JSON text
  YamlDocument doc;
  ASSERT_NO_THROW(doc.Parse(text));

  auto const &root = doc.root();

  EXPECT_TRUE(root.IsObject());
  EXPECT_EQ(root.size(), 8);
  EXPECT_TRUE(root.Has("invoice"));
  EXPECT_TRUE(root.Has("date"));
  EXPECT_TRUE(root.Has("bill-to"));
  EXPECT_TRUE(root.Has("ship-to"));
  EXPECT_TRUE(root.Has("product"));
  EXPECT_TRUE(root.Has("tax"));
  EXPECT_TRUE(root.Has("total"));
  EXPECT_TRUE(root.Has("comments"));

  auto const &invoice = doc["invoice"];
  EXPECT_EQ(invoice.As<int>(), 34843);

  auto const &billTo = doc["bill-to"];
  EXPECT_TRUE(billTo.IsObject());
  EXPECT_TRUE(billTo.Has("given"));
  EXPECT_TRUE(billTo.Has("family"));
  EXPECT_TRUE(billTo.Has("address"));
  EXPECT_EQ(billTo["given"].As<string>(), "Chris");
  EXPECT_EQ(billTo["family"].As<string>(), "Dumars");
  auto const &billToAddr = billTo["address"];
  EXPECT_TRUE(billToAddr.Has("lines"));
  EXPECT_TRUE(billToAddr.Has("city"));
  EXPECT_TRUE(billToAddr.Has("state"));
  EXPECT_TRUE(billToAddr.Has("postal"));
  EXPECT_EQ(billToAddr["lines"].As<string>(), R"(458 Walkman Dr.
Suite #292
)");
  EXPECT_EQ(billToAddr["city"].As<string>(), "Royal Oak");
  EXPECT_EQ(billToAddr["state"].As<string>(), "MI");

  // auto const &shipTo = doc["ship-to"];
}
