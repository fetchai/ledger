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

#include "core/random/lfg.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/counter.hpp"
#include "core/serializers/stl_types.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace fetch::serializers;
using namespace fetch::byte_array;
using namespace std::chrono;

fetch::random::LaggedFibonacciGenerator<> lfg;

template <typename T, std::size_t N = 256>
void MakeString(T &str)
{
  ByteArray entry;
  entry.Resize(256);

  for (std::size_t j = 0; j < 256; ++j)
  {
    entry[j] = uint8_t(lfg() >> 19);
  }

  str = T(entry);
}

template <typename T, std::size_t N = 256>
void MakeStringVector(std::vector<T> &vec, std::size_t size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    T s;
    MakeString(s);
    vec.push_back(s);
  }
}

std::size_t PopulateData(std::vector<uint32_t> &s)
{
  s.resize(16 * 100000);

  for (std::size_t i = 0; i < s.size(); ++i)
  {
    s[i] = uint32_t(lfg());
  }

  return sizeof(uint32_t) * s.size();
}

std::size_t PopulateData(std::vector<uint64_t> &s)
{
  s.resize(16 * 100000);

  for (std::size_t i = 0; i < s.size(); ++i)
  {
    s[i] = lfg();
  }

  return sizeof(uint64_t) * s.size();
}

std::size_t PopulateData(std::vector<ConstByteArray> &s)
{
  MakeStringVector(s, 100000);
  return s[0].size() * s.size();
}

std::size_t PopulateData(std::vector<ByteArray> &s)
{
  MakeStringVector(s, 100000);
  return s[0].size() * s.size();
}

std::size_t PopulateData(std::vector<std::string> &s)
{
  MakeStringVector(s, 100000);
  return s[0].size() * s.size();
}

struct Result
{
  double serialization_time;
  double deserialization_time;
  double serialization;
  double deserialization;
  double size;
};

template <typename S, typename T, typename... Args>
Result BenchmarkSingle(Args... args)
{
  Result      ret;
  T           data;
  std::size_t size = PopulateData(data, args...);

  S buffer;

  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  SizeCounter                    counter;
  counter << data;
  buffer.Reserve(counter.size());
  buffer << data;

  high_resolution_clock::time_point t2 = high_resolution_clock::now();

  T des;
  buffer.seek(0);
  buffer >> des;

  high_resolution_clock::time_point t3  = high_resolution_clock::now();
  duration<double>                  ts1 = duration_cast<duration<double>>(t2 - t1);
  duration<double>                  ts2 = duration_cast<duration<double>>(t3 - t2);
  ret.size                              = double(size) * 1e-6;
  ret.serialization_time                = ts1.count();
  ret.deserialization_time              = ts2.count();
  ret.serialization                     = double(size) * 1e-6 / double(ts1.count());
  ret.deserialization                   = double(size) * 1e-6 / double(ts2.count());
  return ret;
}

#define SINGLE_BENCHMARK(serializer, type)                      \
  result = BenchmarkSingle<serializer, type>();                 \
  std::cout << std::setw(type_width) << #type;                  \
  std::cout << std::setw(width) << result.size;                 \
  std::cout << std::setw(width) << result.serialization_time;   \
  std::cout << std::setw(width) << result.deserialization_time; \
  std::cout << std::setw(width) << result.serialization;        \
  std::cout << std::setw(width) << result.deserialization << std::endl

int main()
{
  int type_width = 35;
  int width      = 12;

  std::cout << std::setw(type_width) << "Type";
  std::cout << std::setw(width) << "MBs";
  std::cout << std::setw(width) << "Ser. time";
  std::cout << std::setw(width) << "Des. time";
  std::cout << std::setw(width) << "Ser. MBs";
  std::cout << std::setw(width) << "Des. MBs" << std::endl;

  Result result;

  SINGLE_BENCHMARK(ByteArrayBuffer, std::vector<uint32_t>);
  SINGLE_BENCHMARK(ByteArrayBuffer, std::vector<uint64_t>);
  SINGLE_BENCHMARK(ByteArrayBuffer, std::vector<ByteArray>);
  SINGLE_BENCHMARK(ByteArrayBuffer, std::vector<ConstByteArray>);
  SINGLE_BENCHMARK(ByteArrayBuffer, std::vector<std::string>);

  std::cout << std::endl;

  std::cout << std::setw(type_width) << "Type";
  std::cout << std::setw(width) << "MBs";
  std::cout << std::setw(width) << "Ser. time";
  std::cout << std::setw(width) << "Des. time";
  std::cout << std::setw(width) << "Ser. MBs";
  std::cout << std::setw(width) << "Des. MBs" << std::endl;

  SINGLE_BENCHMARK(TypedByteArrayBuffer, std::vector<uint32_t>);
  SINGLE_BENCHMARK(TypedByteArrayBuffer, std::vector<uint64_t>);
  SINGLE_BENCHMARK(TypedByteArrayBuffer, std::vector<ByteArray>);
  SINGLE_BENCHMARK(TypedByteArrayBuffer, std::vector<ConstByteArray>);
  SINGLE_BENCHMARK(TypedByteArrayBuffer, std::vector<std::string>);

  return 0;
}
