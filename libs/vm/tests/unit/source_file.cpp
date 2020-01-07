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

#include "core/byte_array/byte_array.hpp"
#include "core/serializers/main_serializer.hpp"
#include "vm/common.hpp"

#include "gtest/gtest.h"

#include <stdexcept>

namespace {

using fetch::serializers::MsgPackSerializer;
using fetch::vm::SourceFile;
using fetch::vm::SourceFiles;

namespace {

const std::string HELLO_WORLD_ETCH = R"(
function main()
  printLn("Hello world!!");
endfunction
)";

const std::string GBYE_WORLD_ETCH = R"(
function main()
  printLn("GoodBye world!!");
endfunction
)";

}  // namespace

class SourceFileSerialization : public ::testing::Test
{
public:
  MsgPackSerializer serializer;
  MsgPackSerializer deserializer;
  SourceFile        source_file_in, source_file_out;
  SourceFiles       source_file_s_in, source_file_s_out;

  void SerializeFrom(std::string const &filename, std::string const &source)
  {
    source_file_in = SourceFile(filename, source);
    serializer << source_file_in;
    deserializer = MsgPackSerializer(serializer.data());
    deserializer >> source_file_out;
  }

  void SerializeFrom(std::vector<SourceFile> const &source_files)
  {
    source_file_s_in = source_files;
    serializer << source_file_s_in;
    deserializer = MsgPackSerializer(serializer.data());
    deserializer >> source_file_s_out;
  }
};

TEST_F(SourceFileSerialization, source_file_single)
{
  std::string filename = "hello_world.etch";
  SerializeFrom(filename, HELLO_WORLD_ETCH);
  ASSERT_EQ(source_file_out.filename, filename);
  ASSERT_EQ(source_file_out.source, HELLO_WORLD_ETCH);
  ASSERT_NE(source_file_out.source, GBYE_WORLD_ETCH);
}

TEST_F(SourceFileSerialization, source_file_vector)
{
  std::vector<SourceFile> source_files{SourceFile{"hw.etch", HELLO_WORLD_ETCH},
                                       SourceFile{"bw.etch", GBYE_WORLD_ETCH}};
  SerializeFrom(source_files);
  std::size_t i = 0;
  for (auto &source_file : source_files)
  {
    ASSERT_EQ(source_file_s_out[i].filename, source_file.filename);
    ASSERT_EQ(source_file_s_out[i].source, source_file.source);
    i++;
  }
}

}  // namespace
