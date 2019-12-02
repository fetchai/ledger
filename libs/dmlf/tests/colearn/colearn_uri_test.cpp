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
#include "dmlf/colearn/colearn_update.hpp"
#include "dmlf/colearn/colearn_uri.hpp"
#include "gtest/gtest.h"

#include <string>

namespace fetch {
namespace dmlf {
namespace colearn {

namespace {
ColearnUpdate::Data byte("");

}  // namespace

TEST(Colearn_URi, defaultConstructor)
{
  ColearnURI uri;

  EXPECT_TRUE(uri.IsEmpty());
  EXPECT_EQ(uri.protocol(), "colearn");
  EXPECT_EQ(uri.owner(), "");
  EXPECT_EQ(uri.algorithm_class(), "");
  EXPECT_EQ(uri.update_type(), "");
  EXPECT_EQ(uri.source(), "");
  EXPECT_EQ(uri.fingerprint(), "");
  EXPECT_EQ(uri.ToString(), "colearn://////");
}

TEST(Colearn_URi, updateConstructor)
{
  ColearnUpdate update{"algo", "type", ColearnUpdate::Data(byte), "source", {}};

  ColearnURI uri{update};

  EXPECT_FALSE(uri.IsEmpty());
  EXPECT_EQ(uri.protocol(), "colearn");
  EXPECT_EQ(uri.owner(), "");
  EXPECT_EQ(uri.algorithm_class(), "algo");
  EXPECT_EQ(uri.update_type(), "type");
  EXPECT_EQ(uri.source(), "source");
  EXPECT_EQ(uri.fingerprint(), "ELM6hjWH59R9Nert8hoZKYNBWY3zubzWGREtR1MurPLe");
  EXPECT_EQ(uri.ToString(),
            "colearn:///algo/type/source/ELM6hjWH59R9Nert8hoZKYNBWY3zubzWGREtR1MurPLe");
}

TEST(Colearn_URi, manualConstruction)
{
  ColearnURI uri;

  uri.owner("owner").algorithm_class("algo").update_type("type").source("source").fingerprint(
      "fingerprint");

  EXPECT_FALSE(uri.IsEmpty());
  EXPECT_EQ(uri.protocol(), "colearn");
  EXPECT_EQ(uri.owner(), "owner");
  EXPECT_EQ(uri.algorithm_class(), "algo");
  EXPECT_EQ(uri.update_type(), "type");
  EXPECT_EQ(uri.source(), "source");
  EXPECT_EQ(uri.fingerprint(), "fingerprint");
  EXPECT_EQ(uri.ToString(), "colearn://owner/algo/type/source/fingerprint");
}

TEST(Colearn_URi, stringConstruction)
{

  {
    std::string asString = "colearn://owner/algo/type/source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_FALSE(uri.IsEmpty());
    EXPECT_EQ(uri.protocol(), "colearn");
    EXPECT_EQ(uri.owner(), "owner");
    EXPECT_EQ(uri.algorithm_class(), "algo");
    EXPECT_EQ(uri.update_type(), "type");
    EXPECT_EQ(uri.source(), "source");
    EXPECT_EQ(uri.fingerprint(), "fingerprint");
    EXPECT_EQ(uri.ToString(), "colearn://owner/algo/type/source/fingerprint");
  }
  {
    std::string asString = "colearn:///algo/type/source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_FALSE(uri.IsEmpty());
    EXPECT_EQ(uri.protocol(), "colearn");
    EXPECT_EQ(uri.owner(), "");
    EXPECT_EQ(uri.algorithm_class(), "algo");
    EXPECT_EQ(uri.update_type(), "type");
    EXPECT_EQ(uri.source(), "source");
    EXPECT_EQ(uri.fingerprint(), "fingerprint");
    EXPECT_EQ(uri.ToString(), "colearn:///algo/type/source/fingerprint");
  }
  {
    std::string asString = "colearn:////type/source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_FALSE(uri.IsEmpty());
    EXPECT_EQ(uri.protocol(), "colearn");
    EXPECT_EQ(uri.owner(), "");
    EXPECT_EQ(uri.algorithm_class(), "");
    EXPECT_EQ(uri.update_type(), "type");
    EXPECT_EQ(uri.source(), "source");
    EXPECT_EQ(uri.fingerprint(), "fingerprint");
    EXPECT_EQ(uri.ToString(), "colearn:////type/source/fingerprint");
  }
  {
    std::string asString = "colearn://///source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_FALSE(uri.IsEmpty());
    EXPECT_EQ(uri.protocol(), "colearn");
    EXPECT_EQ(uri.owner(), "");
    EXPECT_EQ(uri.algorithm_class(), "");
    EXPECT_EQ(uri.update_type(), "");
    EXPECT_EQ(uri.source(), "source");
    EXPECT_EQ(uri.fingerprint(), "fingerprint");
    EXPECT_EQ(uri.ToString(), "colearn://///source/fingerprint");
  }
  {
    std::string asString = "colearn://////fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_FALSE(uri.IsEmpty());
    EXPECT_EQ(uri.protocol(), "colearn");
    EXPECT_EQ(uri.owner(), "");
    EXPECT_EQ(uri.algorithm_class(), "");
    EXPECT_EQ(uri.update_type(), "");
    EXPECT_EQ(uri.source(), "");
    EXPECT_EQ(uri.fingerprint(), "fingerprint");
    EXPECT_EQ(uri.ToString(), "colearn://////fingerprint");
  }
  {
    std::string asString = "colearn://////";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_TRUE(uri.IsEmpty());
    EXPECT_EQ(uri.protocol(), "colearn");
    EXPECT_EQ(uri.owner(), "");
    EXPECT_EQ(uri.algorithm_class(), "");
    EXPECT_EQ(uri.update_type(), "");
    EXPECT_EQ(uri.source(), "");
    EXPECT_EQ(uri.fingerprint(), "");
    EXPECT_EQ(uri.ToString(), "colearn://////");
  }
}

TEST(Colearn_URi, badStringConstruction)
{

  {
    std::string asString = "myuri://owner/algo/type/source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_TRUE(uri.IsEmpty());
  }
  {
    std::string asString = "colearn://owner/source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_TRUE(uri.IsEmpty());
  }
  {
    std::string asString;
    ColearnURI  uri = ColearnURI::Parse(asString);

    EXPECT_TRUE(uri.IsEmpty());
  }
  {
    std::string asString = "AAAAAAA://owner/algo/type/source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_TRUE(uri.IsEmpty());
  }
  {
    std::string asString = "colearn:/owner/algo/type/source/fingerprint";
    ColearnURI  uri      = ColearnURI::Parse(asString);

    EXPECT_TRUE(uri.IsEmpty());
  }
}

}  // namespace colearn
}  // namespace dmlf
}  // namespace fetch
