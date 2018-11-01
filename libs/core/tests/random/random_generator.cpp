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


/* The random number generator was being exercised by the network
   stress tests but this timesout in some environments. We removed the
   random generatoring code from that test and add it here -- we will
   ratio the "random-fill" time aginst a more constant-fill to verify
   that the random code's expense does not creep upward over time.
*/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "core/byte_array/byte_array.hpp"

using ::testing::_;

namespace SpeedTest {
uint32_t GetRandom()
{
  static std::random_device                      rd;
  static std::mt19937                            gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

  return dis(gen);
}

using message_type    = fetch::byte_array::ByteArray;
const int ITERATIONS  = 2;
const int MID_CYCLES  = 10;
const int PACKET_SIZE = 100000;

void GenerateRandom(std::size_t iterations, std::size_t cycles, std::size_t packetsize)
{
  std::vector<message_type> sendData;
  for (std::size_t iteration = 0; iteration < iterations; iteration++)
  {
    for (std::size_t j = 0; j < cycles; j++)
    {
      message_type arr;
      arr.Resize(packetsize);
      for (std::size_t k = 0; k < packetsize; k++)
      {
        arr[k] = uint8_t(GetRandom());
      }
      sendData.push_back(arr);
    }
  }
}

void GenerateConstant(std::size_t iterations, std::size_t cycles, std::size_t packetsize)
{
  std::vector<message_type> sendData;
  for (std::size_t iteration = 0; iteration < iterations; iteration++)
  {
    for (std::size_t j = 0; j < cycles; j++)
    {
      message_type arr;
      arr.Resize(packetsize);
      for (std::size_t k = 0; k < packetsize; k++)
      {
        arr[k] = uint8_t(j + k);
      }
      sendData.push_back(arr);
    }
  }
}

TEST(SpeedTest, CompareRandomSpeed)
{
  auto t1 = std::chrono::system_clock::now();
  GenerateConstant(ITERATIONS, MID_CYCLES, PACKET_SIZE);
  auto t2 = std::chrono::system_clock::now();
  GenerateRandom(ITERATIONS, MID_CYCLES, PACKET_SIZE);
  auto t3 = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed_seconds_c = t2 - t1;
  std::chrono::duration<double> elapsed_seconds_r = t3 - t2;

  std::cout << "Const: " << elapsed_seconds_c.count() << std::endl;
  std::cout << "Rand:  " << elapsed_seconds_r.count() << std::endl;

  auto ratio = elapsed_seconds_r.count() / elapsed_seconds_c.count();
  std::cout << "Ratio:  " << ratio << std::endl;

  ASSERT_LT(ratio, 13.0);
}

}  // namespace SpeedTest
