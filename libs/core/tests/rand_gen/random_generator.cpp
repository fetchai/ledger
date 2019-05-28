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
#include <gtest/gtest.h>

#include <memory>
#include <random>

uint32_t GetRandom()
{
  static std::random_device                      rd;
  static std::mt19937                            gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

  return dis(gen);
}

using message_type    = byte_array::ByteArray;
const int MID_CYCLES  = 50;
const int PACKET_SIZE = 1000000

    class myTestFixture1 : public ::testing::test
{
public:
  myTestFixture1()
  {
    // initialization code here
    exit(77)
  }

  void SetUp()
  {
    // code here will execute just before the test ensues
  }

  void TearDown()
  {
    // code here will be called just after the test completes
    // ok to through exceptions from here if need be
  }

  ~myTestFixture1()
  {
    // cleanup any pending stuff, but no exceptions allowed
  }

  // put in any custom data members that you need
};

TEST_F(myTestFixture1, SpeedTest)
{
  std::vector<message_type> sendData;
  for (std::size_t index = 0; index < 10; ++index)
  {
    for (int j = 0; j < MID_CYCLES; j++)
    {
      for (int k = 0; k < PACKET_SIZE; k++)
      {
        volatile message_type arr;
        arr.Resize(packetSize);
        for (std::size_t z = 0; z < arr.size(); z++)
        {
          //          arr[z] = uint8_t(z+i);
          arr[z] = uint8_t(GetRandom());
        }
        sendData.push_back(arr);
      }
    }
  }

  auto fd = open("/tmp/oink", "w");
  for (auto &d : sendData)
  {
    write(fd, &(d[0]), PACKET_SIZE);
  }
  close(fd);
}
