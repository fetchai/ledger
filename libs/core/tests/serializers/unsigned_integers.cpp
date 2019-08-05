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

TEST(MsgPacker, unsigned_integers)
{
  // Setup
  MsgPackSerializer stream;
  uint64_t          value;

  value  = 0;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("00"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 0);

  value  = 1;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("01"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 1);

  value  = 2;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("02"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 2);

  value  = 3;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("03"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 3);

  value  = 4;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("04"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4);

  value  = 5;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("05"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 5);

  value  = 6;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("06"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 6);

  value  = 7;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("07"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 7);

  value  = 8;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("08"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 8);

  value  = 9;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("09"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 9);

  value  = 10;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("0a"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 10);

  value  = 11;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("0b"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 11);

  value  = 12;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("0c"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 12);

  value  = 13;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("0d"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 13);

  value  = 14;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("0e"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 14);

  value  = 15;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("0f"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 15);

  value  = 16;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("10"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 16);

  value  = 17;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("11"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 17);

  value  = 18;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("12"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 18);

  value  = 19;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("13"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 19);

  value  = 20;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("14"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 20);

  value  = 21;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("15"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 21);

  value  = 22;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("16"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 22);

  value  = 23;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("17"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 23);

  value  = 24;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("18"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 24);

  value  = 25;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("19"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 25);

  value  = 26;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("1a"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 26);

  value  = 27;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("1b"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 27);

  value  = 28;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("1c"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 28);

  value  = 29;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("1d"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 29);

  value  = 30;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("1e"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 30);

  value  = 31;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("1f"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 31);

  value  = 32;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("20"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 32);

  value  = 33;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("21"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 33);

  value  = 34;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("22"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 34);

  value  = 100;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("64"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 100);

  value  = 101;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("65"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 101);

  value  = 102;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("66"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 102);

  value  = 103;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("67"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 103);

  value  = 104;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("68"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 104);

  value  = 105;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("69"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 105);

  value  = 106;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("6a"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 106);

  value  = 107;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("6b"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 107);

  value  = 108;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("6c"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 108);

  value  = 109;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("6d"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 109);

  value  = 110;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("6e"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 110);

  value  = 111;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("6f"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 111);

  value  = 112;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("70"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 112);

  value  = 113;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("71"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 113);

  value  = 114;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("72"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 114);

  value  = 115;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("73"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 115);

  value  = 116;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("74"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 116);

  value  = 117;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("75"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 117);

  value  = 118;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("76"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 118);

  value  = 119;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("77"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 119);

  value  = 120;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("78"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 120);

  value  = 121;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("79"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 121);

  value  = 122;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("7a"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 122);

  value  = 123;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("7b"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 123);

  value  = 124;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("7c"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 124);

  value  = 125;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("7d"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 125);

  value  = 126;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("7e"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 126);

  value  = 127;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("7f"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 127);

  value  = 128;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc80"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 128);

  value  = 129;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc81"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 129);

  value  = 130;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc82"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 130);

  value  = 131;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc83"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 131);

  value  = 132;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc84"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 132);

  value  = 133;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc85"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 133);

  value  = 134;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc86"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 134);

  value  = 135;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc87"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 135);

  value  = 136;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc88"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 136);

  value  = 137;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc89"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 137);

  value  = 138;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc8a"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 138);

  value  = 139;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc8b"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 139);

  value  = 140;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cc8c"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 140);

  value  = 240;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf0"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 240);

  value  = 241;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf1"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 241);

  value  = 242;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf2"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 242);

  value  = 243;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf3"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 243);

  value  = 244;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf4"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 244);

  value  = 245;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf5"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 245);

  value  = 246;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf6"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 246);

  value  = 247;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf7"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 247);

  value  = 248;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf8"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 248);

  value  = 249;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccf9"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 249);

  value  = 250;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccfa"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 250);

  value  = 251;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccfb"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 251);

  value  = 252;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccfc"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 252);

  value  = 253;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccfd"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 253);

  value  = 254;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccfe"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 254);

  value  = 255;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ccff"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 255);

  value  = 256;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0100"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 256);

  value  = 257;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0101"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 257);

  value  = 258;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0102"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 258);

  value  = 259;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0103"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 259);

  value  = 260;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0104"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 260);

  value  = 261;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0105"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 261);

  value  = 262;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0106"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 262);

  value  = 263;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0107"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 263);

  value  = 264;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0108"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 264);

  value  = 265;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0109"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 265);

  value  = 266;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd010a"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 266);

  value  = 267;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd010b"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 267);

  value  = 268;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd010c"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 268);

  value  = 269;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd010d"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 269);

  value  = 270;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd010e"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 270);

  value  = 271;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd010f"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 271);

  value  = 272;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0110"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 272);

  value  = 273;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0111"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 273);

  value  = 274;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cd0112"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 274);

  value  = 65526;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfff6"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65526);

  value  = 65527;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfff7"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65527);

  value  = 65528;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfff8"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65528);

  value  = 65529;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfff9"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65529);

  value  = 65530;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfffa"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65530);

  value  = 65531;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfffb"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65531);

  value  = 65532;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfffc"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65532);

  value  = 65533;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfffd"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65533);

  value  = 65534;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdfffe"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65534);

  value  = 65535;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cdffff"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65535);

  value  = 65536;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010000"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65536);

  value  = 65537;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010001"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65537);

  value  = 65538;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010002"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65538);

  value  = 65539;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010003"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65539);

  value  = 65540;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010004"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65540);

  value  = 65541;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010005"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65541);

  value  = 65542;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010006"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65542);

  value  = 65543;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010007"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65543);

  value  = 65544;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010008"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65544);

  value  = 65545;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ce00010009"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 65545);

  value  = 4294967286;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffff6"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967286);

  value  = 4294967287;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffff7"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967287);

  value  = 4294967288;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffff8"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967288);

  value  = 4294967289;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffff9"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967289);

  value  = 4294967290;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffffa"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967290);

  value  = 4294967291;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffffb"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967291);

  value  = 4294967292;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffffc"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967292);

  value  = 4294967293;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffffd"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967293);

  value  = 4294967294;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cefffffffe"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967294);

  value  = 4294967295;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("ceffffffff"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967295);

  value  = 4294967296;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000000"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967296);

  value  = 4294967297;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000001"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967297);

  value  = 4294967298;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000002"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967298);

  value  = 4294967299;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000003"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967299);

  value  = 4294967300;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000004"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967300);

  value  = 4294967301;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000005"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967301);

  value  = 4294967302;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000006"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967302);

  value  = 4294967303;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000007"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967303);

  value  = 4294967304;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000008"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967304);

  value  = 4294967305;
  stream = MsgPackSerializer();
  stream << value;
  EXPECT_EQ(FromHex("cf0000000100000009"), stream.data());
  stream.seek(0);
  value = 0;
  stream >> value;
  EXPECT_EQ(value, 4294967305);
}

}  // namespace serializers
}  // namespace fetch
