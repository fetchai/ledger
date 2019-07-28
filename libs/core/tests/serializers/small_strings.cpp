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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/group_definitions.hpp"
#include "core/serializers/main_serializer.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>

using namespace fetch::byte_array;

namespace fetch {

namespace serializers {

TEST(MsgPacker, short_strings)
{
  // Setup
  MsgPackSerializer stream;
  ConstByteArray    value;

  // len(value) = 0
  value  = "";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a0"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "");

  // len(value) = 1
  value  = "L";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a14c"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "L");

  // len(value) = 2
  value  = "Lo";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a24c6f"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lo");

  // len(value) = 3
  value  = "Lor";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a34c6f72"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lor");

  // len(value) = 4
  value  = "Lore";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a44c6f7265"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lore");

  // len(value) = 5
  value  = "Lorem";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a54c6f72656d"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem");

  // len(value) = 6
  value  = "Lorem ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a64c6f72656d20"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ");

  // len(value) = 7
  value  = "Lorem i";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a74c6f72656d2069"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem i");

  // len(value) = 8
  value  = "Lorem ip";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a84c6f72656d206970"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ip");

  // len(value) = 9
  value  = "Lorem ips";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("a94c6f72656d20697073"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ips");

  // len(value) = 10
  value  = "Lorem ipsu";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("aa4c6f72656d2069707375"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsu");

  // len(value) = 11
  value  = "Lorem ipsum";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ab4c6f72656d20697073756d"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum");

  // len(value) = 12
  value  = "Lorem ipsum ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ac4c6f72656d20697073756d20"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum ");

  // len(value) = 13
  value  = "Lorem ipsum d";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ad4c6f72656d20697073756d2064"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum d");

  // len(value) = 14
  value  = "Lorem ipsum do";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ae4c6f72656d20697073756d20646f"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum do");

  // len(value) = 15
  value  = "Lorem ipsum dol";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("af4c6f72656d20697073756d20646f6c"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dol");

  // len(value) = 16
  value  = "Lorem ipsum dolo";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b04c6f72656d20697073756d20646f6c6f"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolo");

  // len(value) = 17
  value  = "Lorem ipsum dolor";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b14c6f72656d20697073756d20646f6c6f72"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor");

  // len(value) = 18
  value  = "Lorem ipsum dolor ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b24c6f72656d20697073756d20646f6c6f7220"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor ");

  // len(value) = 19
  value  = "Lorem ipsum dolor s";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b34c6f72656d20697073756d20646f6c6f722073"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor s");

  // len(value) = 20
  value  = "Lorem ipsum dolor si";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b44c6f72656d20697073756d20646f6c6f72207369"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor si");

  // len(value) = 21
  value  = "Lorem ipsum dolor sit";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b54c6f72656d20697073756d20646f6c6f7220736974"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit");

  // len(value) = 22
  value  = "Lorem ipsum dolor sit ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b64c6f72656d20697073756d20646f6c6f722073697420"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit ");

  // len(value) = 23
  value  = "Lorem ipsum dolor sit a";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b74c6f72656d20697073756d20646f6c6f72207369742061"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit a");

  // len(value) = 24
  value  = "Lorem ipsum dolor sit am";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b84c6f72656d20697073756d20646f6c6f722073697420616d"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit am");

  // len(value) = 25
  value  = "Lorem ipsum dolor sit ame";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("b94c6f72656d20697073756d20646f6c6f722073697420616d65"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit ame");

  // len(value) = 26
  value  = "Lorem ipsum dolor sit amet";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ba4c6f72656d20697073756d20646f6c6f722073697420616d6574"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet");

  // len(value) = 27
  value  = "Lorem ipsum dolor sit amet,";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("bb4c6f72656d20697073756d20646f6c6f722073697420616d65742c"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet,");

  // len(value) = 28
  value  = "Lorem ipsum dolor sit amet, ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("bc4c6f72656d20697073756d20646f6c6f722073697420616d65742c20"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet, ");

  // len(value) = 29
  value  = "Lorem ipsum dolor sit amet, c";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("bd4c6f72656d20697073756d20646f6c6f722073697420616d65742c2063"), stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet, c");

  // len(value) = 30
  value  = "Lorem ipsum dolor sit amet, co";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("be4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet, co");

  // len(value) = 31
  value  = "Lorem ipsum dolor sit amet, con";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("bf4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet, con");

  // len(value) = 32
  value  = "Lorem ipsum dolor sit amet, cons";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9204c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet, cons");

  // len(value) = 33
  value  = "Lorem ipsum dolor sit amet, conse";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9214c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet, conse");

  // len(value) = 34
  value  = "Lorem ipsum dolor sit amet, consec";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9224c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value, "Lorem ipsum dolor sit amet, consec");

  // len(value) = 100
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9644c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed");

  // len(value) = 101
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9654c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e6775652073656420"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed ");

  // len(value) = 102
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed l";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9664c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed l");

  // len(value) = 103
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed le";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9674c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c65"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed le");

  // len(value) = 104
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9684c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo");

  // len(value) = 105
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9694c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo ");

  // len(value) = 106
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo i";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d96a4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f2069"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo i");

  // len(value) = 107
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d96b4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in");

  // len(value) = 108
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d96c4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in ");

  // len(value) = 109
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in p";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d96d4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e2070"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in p");

  // len(value) = 110
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in pl";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d96e4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in pl");

  // len(value) = 111
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in pla";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d96f4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in pla");

  // len(value) = 112
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in plac";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9704c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in plac");

  // len(value) = 113
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in place";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9714c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c616365"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in place");

  // len(value) = 114
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placer";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9724c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placer");

  // len(value) = 115
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placera";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9734c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placera");

  // len(value) = 116
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9744c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c616365726174"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat");

  // len(value) = 117
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat.";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9754c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat.");

  // len(value) = 118
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9764c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e20"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. ");

  // len(value) = 119
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. M";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9774c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. M");

  // len(value) = 120
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Ma";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9784c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Ma");

  // len(value) = 121
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mau";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9794c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mau");

  // len(value) = 122
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Maur";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d97a4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Maur");

  // len(value) = 123
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauri";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d97b4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauri");

  // len(value) = 124
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d97c4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris");

  // len(value) = 125
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d97d4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris ");

  // len(value) = 126
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris e";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d97e4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269732065"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris e");

  // len(value) = 127
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d97f4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973206574"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et");

  // len(value) = 128
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9804c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320657420"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et ");

  // len(value) = 129
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et e";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9814c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269732065742065"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et e");

  // len(value) = 130
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et el";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9824c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et el");

  // len(value) = 131
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et eli";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9834c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c69"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et eli");

  // len(value) = 132
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9844c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c6974"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit");

  // len(value) = 133
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9854c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c697420"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit ");

  // len(value) = 134
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit i";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9864c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c69742069"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit i");

  // len(value) = 135
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9874c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c697420696e"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in");

  // len(value) = 136
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9884c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c697420696e20"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in ");

  // len(value) = 137
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in q";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d9894c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c697420696e2071"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in q");

  // len(value) = 138
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in qu";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d98a4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c697420696e207175"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in qu");

  // len(value) = 139
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in qua";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d98b4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c697420696e20717561"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in qua");

  // len(value) = 140
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("d98c4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574"
                    "75722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64"
                    "696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261"
                    "742e204d617572697320657420656c697420696e207175616d"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam");

  // len(value) = 240
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neq";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f04c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e6571"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neq");

  // len(value) = 241
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris nequ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f14c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e657175"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris nequ");

  // len(value) = 242
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f24c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e65717565"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque");

  // len(value) = 243
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f34c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e6571756520"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque ");

  // len(value) = 244
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque f";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f44c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e657175652066"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque f");

  // len(value) = 245
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque fe";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f54c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e65717565206665"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque fe");

  // len(value) = 246
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque fel";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f64c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque fel");

  // len(value) = 247
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque feli";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f74c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c69"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque feli");

  // len(value) = 248
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f84c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c6973"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis");

  // len(value) = 249
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis,";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("d9f94c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e736563746574757220"
              "61646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068"
              "6173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d6175726973"
              "20657420656c697420696e207175616d20756c7472696365732076756c70757461746520757420616320"
              "6a7573746f2e20446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d"
              "6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c69732c"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis,");

  // len(value) = 250
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9fa4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320657420656c"
          "697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e2044"
          "6f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e64726572"
          "697420666575676961742e204d6175726973206e657175652066656c69732c20"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, ");

  // len(value) = 251
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, e";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9fb4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320657420656c"
          "697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e2044"
          "6f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e64726572"
          "697420666575676961742e204d6175726973206e657175652066656c69732c2065"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, e");

  // len(value) = 252
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, el";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9fc4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320657420656c"
          "697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e2044"
          "6f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e64726572"
          "697420666575676961742e204d6175726973206e657175652066656c69732c20656c"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, el");

  // len(value) = 253
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, ele";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9fd4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320657420656c"
          "697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e2044"
          "6f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e64726572"
          "697420666575676961742e204d6175726973206e657175652066656c69732c20656c65"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, ele");

  // len(value) = 254
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elem";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9fe4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320657420656c"
          "697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e2044"
          "6f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e64726572"
          "697420666575676961742e204d6175726973206e657175652066656c69732c20656c656d"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elem");

  // len(value) = 255
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, eleme";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "d9ff4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572206164"
          "6970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050686173656c"
          "6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d617572697320657420656c"
          "697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e2044"
          "6f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e64726572"
          "697420666575676961742e204d6175726973206e657175652066656c69732c20656c656d65"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, eleme");

  // len(value) = 256
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elemen";
  stream = MsgPackSerializer();
  stream << value;

  EXPECT_EQ(
      FromHex(
          "da01004c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73656374657475722061"
          "646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068617365"
          "6c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269732065742065"
          "6c697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e20"
          "446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e647265"
          "72697420666575676961742e204d6175726973206e657175652066656c69732c20656c656d656e"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elemen");

  // len(value) = 257
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, element";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "da01014c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73656374657475722061"
          "646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068617365"
          "6c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269732065742065"
          "6c697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e20"
          "446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e647265"
          "72697420666575676961742e204d6175726973206e657175652066656c69732c20656c656d656e74"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, element");

  // len(value) = 258
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementu";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "da01024c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73656374657475722061"
          "646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068617365"
          "6c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269732065742065"
          "6c697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e20"
          "446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e647265"
          "72697420666575676961742e204d6175726973206e657175652066656c69732c20656c656d656e7475"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementu");

  // len(value) = 259
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "da01034c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73656374657475722061"
          "646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068617365"
          "6c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269732065742065"
          "6c697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e20"
          "446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e647265"
          "72697420666575676961742e204d6175726973206e657175652066656c69732c20656c656d656e74756d"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum");

  // len(value) = 260
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex(
          "da01044c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e73656374657475722061"
          "646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e205068617365"
          "6c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269732065742065"
          "6c697420696e207175616d20756c7472696365732076756c707574617465207574206163206a7573746f2e20"
          "446f6e6563206120706f727461206f7263692e2043757261626974757220657569736d6f642068656e647265"
          "72697420666575676961742e204d6175726973206e657175652066656c69732c20656c656d656e74756d20"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum ");

  // len(value) = 261
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum v";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da01054c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d2076"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum v");

  // len(value) = 262
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vi";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da01064c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d207669"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vi");

  // len(value) = 263
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vit";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da01074c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d20766974"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vit");

  // len(value) = 264
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vita";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da01084c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d2076697461"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vita");

  // len(value) = 265
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da01094c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d207669746165"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae");

  // len(value) = 266
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da010a4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d20766974616520"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae ");

  // len(value) = 267
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae m";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da010b4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d207669746165206d"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae m");

  // len(value) = 268
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae ma";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da010c4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d207669746165206d61"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae ma");

  // len(value) = 269
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae mas";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("da010d4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465"
                    "7475722061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f"
                    "64696f2e2050686173656c6c757320636f6e67756520736564206c656f20696e20706c61636572"
                    "61742e204d617572697320657420656c697420696e207175616d20756c7472696365732076756c"
                    "707574617465207574206163206a7573746f2e20446f6e6563206120706f727461206f7263692e"
                    "2043757261626974757220657569736d6f642068656e64726572697420666575676961742e204d"
                    "6175726973206e657175652066656c69732c20656c656d656e74756d207669746165206d6173"),
            stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae mas");

  // len(value) = 270
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae mass";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("da010e4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572"
              "2061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050"
              "686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269"
              "7320657420656c697420696e207175616d20756c7472696365732076756c707574617465207574206163"
              "206a7573746f2e20446f6e6563206120706f727461206f7263692e204375726162697475722065756973"
              "6d6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c69732c"
              "20656c656d656e74756d207669746165206d617373"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae mass");

  // len(value) = 271
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae massa";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("da010f4c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572"
              "2061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050"
              "686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269"
              "7320657420656c697420696e207175616d20756c7472696365732076756c707574617465207574206163"
              "206a7573746f2e20446f6e6563206120706f727461206f7263692e204375726162697475722065756973"
              "6d6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c69732c"
              "20656c656d656e74756d207669746165206d61737361"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(
      value,
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae massa");

  // len(value) = 272
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae massa ";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("da01104c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572"
              "2061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050"
              "686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269"
              "7320657420656c697420696e207175616d20756c7472696365732076756c707574617465207574206163"
              "206a7573746f2e20446f6e6563206120706f727461206f7263692e204375726162697475722065756973"
              "6d6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c69732c"
              "20656c656d656e74756d207669746165206d6173736120"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, "
            "elementum vitae massa ");

  // len(value) = 273
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae massa "
      "a";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("da01114c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572"
              "2061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050"
              "686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269"
              "7320657420656c697420696e207175616d20756c7472696365732076756c707574617465207574206163"
              "206a7573746f2e20446f6e6563206120706f727461206f7263692e204375726162697475722065756973"
              "6d6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c69732c"
              "20656c656d656e74756d207669746165206d617373612061"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, "
            "elementum vitae massa a");

  // len(value) = 274
  value =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. Phasellus "
      "congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac justo. Donec a "
      "porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, elementum vitae massa "
      "a,";
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(
      FromHex("da01124c6f72656d20697073756d20646f6c6f722073697420616d65742c20636f6e7365637465747572"
              "2061646970697363696e6720656c69742e20446f6e65632076656c2074656d706f72206f64696f2e2050"
              "686173656c6c757320636f6e67756520736564206c656f20696e20706c6163657261742e204d61757269"
              "7320657420656c697420696e207175616d20756c7472696365732076756c707574617465207574206163"
              "206a7573746f2e20446f6e6563206120706f727461206f7263692e204375726162697475722065756973"
              "6d6f642068656e64726572697420666575676961742e204d6175726973206e657175652066656c69732c"
              "20656c656d656e74756d207669746165206d6173736120612c"),
      stream.data());
  stream.seek(0);
  value = "";
  stream >> value;
  EXPECT_EQ(value,
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vel tempor odio. "
            "Phasellus congue sed leo in placerat. Mauris et elit in quam ultrices vulputate ut ac "
            "justo. Donec a porta orci. Curabitur euismod hendrerit feugiat. Mauris neque felis, "
            "elementum vitae massa a,");
}

}  // namespace serializers
}  // namespace fetch
