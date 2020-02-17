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
#include "bloom_filter/bloom_filter.hpp"

#include <iostream>
#include <fstream>

using fetch::BasicBloomFilter;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;

ConstByteArray ReadHash(std::ifstream &stream)
{
  ByteArray buffer{};
  buffer.Resize(32);

  stream.read(buffer.char_pointer(), static_cast<std::streamsize>(buffer.size()));

  return {buffer};
}

uint64_t ToU64(char const *value)
{
  uint64_t output = std::strtoull(value, nullptr, 10);

  if (errno == ERANGE)
  {
    errno = 0;
    output = 0;
  }

  return output;
}

int main(int argc, char const * const *argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: bloom-tool <filter size> <hashes file>" << std::endl;
    return EXIT_FAILURE;
  }

  // load up the binary hash file
  std::size_t const filter_size    = ToU64(argv[1]);
  char const *      input_filename = argv[2];

  // create the input stream
  std::ifstream input_file{input_filename, std::ios::binary | std::ios::in};

  // try adding the hashes to the filter
  BasicBloomFilter filter{filter_size};

  // read all the data
  std::size_t false_positives{0};
  std::size_t total_hashes{0};
  std::size_t bit_match_high_water_mark{0};

  while (!input_file.eof())
  {
    // read the hash from disk
    auto const hash = ReadHash(input_file);

    ++total_hashes;

    // check the bloom filter
    auto const result = filter.Match(hash);

    if (result.first)
    {
      std::cout << "False Positive for 0x" << hash.ToHex() << " bit: " << result.second
                << " (count: " << total_hashes << ")" << std::endl;
      ++false_positives;
      continue; // don't unduely fill up bloom
    }

    if (result.second > bit_match_high_water_mark)
    {
      std::cout << "High water mark update @ " << total_hashes << " value: " << result.second << std::endl;
      bit_match_high_water_mark = result.second;
    }

    // add the hash to the filter
    filter.Add(hash);
  }

  std::cout << "\n\nSummary: total: " << total_hashes << " false positives: " << false_positives << std::endl;

  return EXIT_SUCCESS;
}