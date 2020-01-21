#pragma once
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

#include "meta/type_traits.hpp"

#include <cstdint>
#include <fstream>

namespace fetch {
namespace bloom {
namespace details {

struct MetaDataHeader
{
  uint64_t version;
  uint64_t length;
};

template <typename T>
meta::IfIsPod<T> WriteToStream(std::ostream &stream, T const &element)
{
  auto const *raw = reinterpret_cast<char const *>(&element);
  stream.write(raw, sizeof(T));
}

template <typename T>
meta::IfIsPod<T> ReadFromStream(std::istream &stream, T &element)
{
  auto *raw = reinterpret_cast<char *>(&element);
  stream.read(raw, sizeof(T));
}

} // namespace details

enum class MetaReadStatus
{
  SUCCESS,
  FAILED,
  NO_FILE_PRESENT,
  VERSION_MISMATCH
};

template <typename T>
MetaReadStatus LoadMetadataFromFile(char const *filename, T &metadata, uint64_t expected_version)
{
  static constexpr uint64_t EXPECTED_FILE_SIZE = (sizeof(T) + sizeof(details::MetaDataHeader));

  MetaReadStatus status{MetaReadStatus::FAILED};

  std::ifstream stream(filename, std::ios::in | std::ios::binary | std::ios::ate);
  if (stream.is_open())
  {
    auto const file_size = static_cast<uint64_t>(stream.tellg());

    // ensure the file size is correct
    if (file_size >= sizeof(details::MetaDataHeader))
    {
      // rewind to the start of the file
      stream.seekg(0);

      // read the metadata contents
      details::MetaDataHeader header{};
      details::ReadFromStream(stream, header);

      // check the version and size
      bool const is_version_correct     = header.version == expected_version;
      bool const is_header_size_correct = header.length == sizeof(T);
      bool const is_file_size_correct   = file_size == EXPECTED_FILE_SIZE;

      if (is_version_correct && is_header_size_correct && is_file_size_correct)
      {
        details::ReadFromStream(stream, metadata);

        status = MetaReadStatus::SUCCESS;
      }
      else
      {
        status = MetaReadStatus::VERSION_MISMATCH;
      }
    }
  }
  else
  {
    status = MetaReadStatus::NO_FILE_PRESENT;
  }

  return status;
}

template <typename T>
bool SaveMetadataToFile(char const *filename, T const &metadata, uint64_t version)
{
  bool success{false};

  std::ofstream stream(filename, std::ios::out | std::ios::binary | std::ios::ate);
  if (stream.is_open())
  {
    // seek past the header
    stream.seekp(sizeof(details::MetaDataHeader));

    // write out the contents specified
    details::WriteToStream(stream, metadata);

    // generate and update the header
    details::MetaDataHeader header{};
    header.version = version;
    header.length = sizeof(T);

    stream.seekp(0);
    details::WriteToStream(stream, header);

    success = true;
  }

  return success;
}

} // namespace bloom
} // namespace fetch