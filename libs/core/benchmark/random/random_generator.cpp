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

/* The random number generator was being exercised by the network
   stress tests but this timed out in some environments. We removed the
   random generatoring code from that test and add it here -- we will
   ratio the "random-fill" time aginst a more constant-fill to verify
   that the random code's expense does not creep upward over time...
*/

#include <benchmark/benchmark.h>

#include <memory>
#include <random>

#include "core/byte_array/byte_array.hpp"

namespace {

using ByteArray       = fetch::byte_array::ByteArray;
const int ITERATIONS  = 2;
const int MID_CYCLES  = 10;
const int PACKET_SIZE = 100000;

uint32_t GetRandom()
{
  static std::random_device                      rd;
  static std::mt19937                            gen(rd());
  static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());

  return dis(gen);
}

void GenerateRandom(std::size_t iterations, std::size_t cycles, std::size_t packet_size)
{
  std::vector<ByteArray> sendData;
  for (std::size_t iteration = 0; iteration < iterations; iteration++)
  {
    for (std::size_t j = 0; j < cycles; j++)
    {
      ByteArray arr;
      arr.Resize(packet_size);
      for (std::size_t k = 0; k < packet_size; k++)
      {
        arr[k] = uint8_t(GetRandom());
      }
      sendData.push_back(arr);
    }
  }
}

void GenerateConstant(std::size_t iterations, std::size_t cycles, std::size_t packet_size)
{
  std::vector<ByteArray> sendData;
  for (std::size_t iteration = 0; iteration < iterations; iteration++)
  {
    for (std::size_t j = 0; j < cycles; j++)
    {
      ByteArray arr;
      arr.Resize(packet_size);
      for (std::size_t k = 0; k < packet_size; k++)
      {
        arr[k] = uint8_t(j + k);
      }
      sendData.push_back(arr);
    }
  }
}

void BenchmarkConstant(benchmark::State &state)
{
  for (auto _ : state)
  {
    GenerateConstant(ITERATIONS, MID_CYCLES, PACKET_SIZE);
  }
}

void BenchmarkRandom(benchmark::State &state)
{
  for (auto _ : state)
  {
    GenerateRandom(ITERATIONS, MID_CYCLES, PACKET_SIZE);
  }
}

BENCHMARK(BenchmarkRandom);
BENCHMARK(BenchmarkConstant);

}  // namespace
