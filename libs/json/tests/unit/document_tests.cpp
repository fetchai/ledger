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

#include "json/document.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <sstream>

using fetch::json::JSONDocument;

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
  EXPECT_TRUE(doc["a"].IsInteger());
  EXPECT_TRUE(doc["b"].IsFloatingPoint());
  EXPECT_TRUE(doc["c"].IsFloatingPoint());
  EXPECT_TRUE(doc["d"].IsString());
  EXPECT_TRUE(doc["e"].IsNull());
  EXPECT_TRUE(doc["f"].IsBoolean());
  EXPECT_TRUE(doc["g"].IsBoolean());
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
      if (i != 0u)
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

TEST(JsonTests, ExpectedToThrow)
{
  char const *text =
      R"({'version': 3, 'consensus': {'startTime': 1573502318, 'cabinetSize': 20, 'stakers': [{'identity': '5iLC924/7d9JvfqMSvLiqx03BvkHy/uI+wDeQF/hvWqVUcAZ2QG+LHkIUTQSo3VZKIxj+qT+YGDOSCPYGrEqTA==', 'amount': 10000}, {'identity': '6Uqc0HXVxn3mNIxKaWqO62b7IpPde6d6TEusxwEmeBQdNOzTXkkZejG/OyZHmMdy7o4s94LlGcHAgfA43/OG7g==', 'amount': 10000}, {'identity': 'ywsajlpaI/Ezvq8foUtndl7C+N9F+ssh9u1sP5j0YtiCBau9FYCK5SwZle2qJSWKauBOdlYqcsRHodxFYS3Eqg==', 'amount': 10000}, {'identity': 'fnpVGM5Q2xdhhHPYsPpkLmtuNYSgMPV//7CjSKX4B5btp0hgNPVIFoeOwHVhWOWycx4kZT167w72QvECuWu5iA==', 'amount': 10000}, {'identity': '9OLlhZ5DoU9ZQLnh5HnK3N0i6AY+2OF2CJgdXRfH8R/lfcq+aPS3igjwMiN0T2dOMzEko6hslOKSnZlVSxO4JQ==', 'amount': 10000}, {'identity': 'LVZHTdnafboFuE8yYwGQMFN/WcE67jAvP8Ry+k2hO15zpJDyvbGvDwyPHChh9Ay0DkF+ns41f6Z1DKn0i7mvFQ==', 'amount': 10000}, {'identity': 'qKUefaAk5Y+rixouggddkoyf6JjBGVxfHU6TisVMVL08DkOwAKqpz154Tzg2y4ydT2UpF8DvNWXqObJtrQ1lsQ==', 'amount': 10000}, {'identity': 'rk4kFp+b2i8SeHZLP44Ir/O23dWj6Q1TAtKogcnUEzZr8A6IC59agLoHHf/zGEwshVupzFrbxV7mb7kNLqZxqw==', 'amount': 10000}, {'identity': '0iOSuqMHWmJ1B6Rlq1GPDvRUf+VYUywviYx+3Coi7mn/zgv+27qd1FrNdQtCO0CgO/+O2Ytz+Ryzfk7+lmIaqQ==', 'amount': 10000}, {'identity': 'vs8VNm6RzFQdLbFR3OxexhAQlPYCG7tfSvtXcKxuwTjUDVV3i3tHbwyJ8i2nO5rR0aNWNFdv2mq2lgAUaqKq3Q==', 'amount': 10000}, {'identity': '2af2UjlFULimbIGpUZBM2FQcTML8bLHT11zEY8mcb166sxtRw20wKJrD9k3jBDMqe8qj7sTO8PDD8ma9lYJ52A==', 'amount': 10000}, {'identity': 'Lfcjim3lEMujN0bu4lCNUxuSRyvuL8MOXnPVipjxFImdH2c/+K40sOpDck2ut2WzCFfCdNSEoVPk5xtWJuBh6g==', 'amount': 10000}, {'identity': 'UM6E/6Xepa7DsBpalJuDIdnRIfCGFBGA271PG8k9V6p98P2ghT9xw5HN4tRRtLA0I9xS3zG0Y2QGvlJAhuW/JA==', 'amount': 10000}, {'identity': 'md3f25bLHEuxkGaovywhVwijSyReft/JUCyZrcHOd1sFA6XWBgEptRhLPoMmjHObRV5bXTjMurFoNH975VZ/5w==', 'amount': 10000}, {'identity': 'NbOyvlgnxu7M5WX2Vt5lW7XWTbw7U6JCYuIzLMlQ9F0ar8pz9PLx6f003ALIR0actyAfNh9ITmO6EiQ+tIEOfw==', 'amount': 10000}, {'identity': 'WYOfQwjwRdV7uh+ZQ6Gr5fzHHi7UUiaZVxMDyCLbFgrSiUlGTB7DUcjmrjPMl5PpTBY16DA7mCS0HQHqLqMqiw==', 'amount': 10000}, {'identity': 'hHD6WE1Cqf/aPqc39sK3rfZvJhq9WikMkgPkoJA+pHjB3TeayAM2p8QOPm5+kAJTW8/gHqQyqWnz32FDnaoYYA==', 'amount': 10000}, {'identity': 'PeWv11i9qUHKqoQ64+9ukLhrhLbBzxi72bjcgk8q7GEChvJYL2ScsZUwBjc/o63aetzY8sQUoEJS7pglan4ElQ==', 'amount': 10000}, {'identity': '6J4gscpRXDZakJSqKEGD+aR+h0Esx9p5MZa/aOPpgTzxnkBa3h1NwSJCfYH1F2XIhZmlbWmkcqm6oHqHbJxgDA==', 'amount': 10000}, {'identity': 'PGqZTXud+lJDvoeVRrW4UCoazhYVjad1GkQlr/Ji+vdcU6A748j2WaohBxGUuGMeLtomqrsNGQPcI+lEpY6LXw==', 'amount': 10000}], 'entropyRunahead': 2, 'aeonPeriodicity': 25, 'aeonOffset': 100, 'minimumStake': 1000}, 'accounts': [{'key': 'bbl68GNae0t0SEAK5XjM0oeQmbeL3umXiABIbqdsZgs=', 'balance': 100, 'stake': 10000}, {'key': 'LpwY4F2gstP0S43652iq1aq9LK/IMX9MPDhNIcpqe84=', 'balance': 100, 'stake': 10000}, {'key': 'ctRw6rlvDVyJwMrK5sAgk7YrIJbBiWrYltK/o60fuZI=', 'balance': 100, 'stake': 10000}, {'key': '6It0SPRHlAh37dz9ZI7WsoVghd3YXub/TsWsi96YDU4=', 'balance': 100, 'stake': 10000}, {'key': '+XFPX/4rKprZpk6xDF4f/4Mj2Fiw9rCarF1dBjt8Few=', 'balance': 100, 'stake': 10000}, {'key': 'ItRcGSesOwe90GWHToFusmbBlYo8A7AJIA64exild6Q=', 'balance': 100, 'stake': 10000}, {'key': '+k3UTFvZl/IaP4NlpAbyHYT+dLVYGM7GP0QviGHSsK0=', 'balance': 100, 'stake': 10000}, {'key': 'rjHrzHdPNsOXD27g5NQ9gP6muSIWCB8dJBb3BVPLsjE=', 'balance': 100, 'stake': 10000}, {'key': 'kajakSd7dtuq9eOo2xxF64tYc3HkaMFcaof0OyuOg5U=', 'balance': 100, 'stake': 10000}, {'key': 'OGwe9zVnSMwWEC30MWF8PwNofbDbWTKerz4AcoT/3nU=', 'balance': 100, 'stake': 10000}, {'key': 'Jt2APIuYx/x1c5nKaRnbkfYQSYgl0AYuRX3FfYrVie4=', 'balance': 100, 'stake': 10000}, {'key': 'PEofL4u/yreGvRNMaUJE9aJ1bPGBoGNZ/joCagSNKhU=', 'balance': 100, 'stake': 10000}, {'key': 'Gw+wnSn64gzF2QS8OPwPx+LscRL+MA+B69+IMZsOEEU=', 'balance': 100, 'stake': 10000}, {'key': 'zmqqvKpJmcNcZkrZM2UrwwdsnWkSy3r0jr3eCfZDhH4=', 'balance': 100, 'stake': 10000}, {'key': 'tdrQM/7xQAfpoAHa/rNNzYDBtdPJ8Gc7vo0D0IT+l/8=', 'balance': 100, 'stake': 10000}, {'key': '+dIR/JIfx1YwFBF+HgulTH22KZraMBAf8O8kZGTdwMw=', 'balance': 100, 'stake': 10000}, {'key': 'fq2bV8XRlw4cgUGLCYf8jUMs8LOhYvsS2ehppwxnOf8=', 'balance': 100, 'stake': 10000}, {'key': 'Ueq4dQ1+HS62d17NN181bylt7M/YA3omCD7QZqKsPig=', 'balance': 100, 'stake': 10000}, {'key': 'wXCL0zZX31zIX1qC2Mc6BgRVbabBIkrGbycdNjEm1t4=', 'balance': 100, 'stake': 10000}, {'key': 'KN4OQiJkMAQ0Necmwha5bSjL+/LbnftZMVJdcM5Mxsg=', 'balance': 100, 'stake': 10000}]})";

  // parse the JSON text
  JSONDocument doc;
  EXPECT_THROW(doc.Parse(text), fetch::json::JSONParseException);
}
