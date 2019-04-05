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
#include "core/byte_array/encoders.hpp"
#include "storage/file_object.hpp"
#include "storage/key_value_index.hpp"
#include <iostream>
#include <map>

#include <gtest/gtest.h>
using namespace fetch;
using namespace fetch::storage;

#define BLOCK_SIZE 2048

using file_block_type      = FileBlockType<BLOCK_SIZE>;
using key_value_index_type = KeyValueIndex<>;
using file_store_type      = VersionedRandomAccessStack<file_block_type>;
using file_object_type     = FileObject<file_store_type>;

TEST(storage_versioned_file_object_gtest, hash_test)
{
  file_store_type fs;
  fs.New("a.db", "b.db");
  uint64_t record;
  {  //
    file_object_type fobj(fs);
    record = fobj.id();
  }

  std::map<std::size_t, byte_array::ByteArray> expected_hashes;
  for (std::size_t j = 0; j < 100; ++j)
  {
    //    std::cout << std::endl;
    //    std::cout << "Round " << j << std::endl;
    //    std::cout << "=========" << std::endl;

    std::size_t N = j * 10;

    for (std::size_t i = N; i < (N + 10); ++i)
    {
      {
        file_object_type fobj(fs, record);
        expected_hashes[i] = fobj.Hash();

        //        std::cout << byte_array::ToBase64(fobj.Hash()) << std::endl;
        fobj.Seek(fobj.size());
        fobj.Write("hello world");
        fobj.Seek(0);

        byte_array::ByteArray x;
        x.Resize(fobj.size());
        fobj.Read(x);
        //        std::cout << "DATA:" << x << std::endl;
      }
      //      std::cout << std::endl;
      //      std::cout << "Commiting " << i << std::endl;
      //      std::cout << "=====================" << std::endl;
      //      std::cout << "Hash: " << byte_array::ToBase64(expected_hashes[i]) << std::endl;

      fs.Commit(i + 1);
    }

    for (std::size_t i = (N + 9); i > (N + 5); --i)
    {
      //      std::cout << std::endl;
      //      std::cout << "Reverting to " << i << std::endl;
      //      std::cout << "=====================" << std::endl;
      //      std::cout << "Expecting " << byte_array::ToBase64(expected_hashes[i]) << std::endl;
      fs.Revert(i);
      {
        file_object_type fobj(fs, record);
        ASSERT_EQ(expected_hashes[i], fobj.Hash()) << "Expectation not met!";

        //        std::cout << "Got: " << byte_array::ToBase64(fobj.Hash()) << std::endl;
        byte_array::ByteArray x;
        x.Resize(fobj.size());
        fobj.Read(x);
        //        std::cout << "DATA:" << x << std::endl;
      }
    }
  }
}