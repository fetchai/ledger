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

#include "storage/file_object.hpp"
#include "core/random/lfg.hpp"

#include <crypto/hash.hpp>
#include <gtest/gtest.h>

using namespace fetch;
using namespace fetch::storage;
using namespace fetch::byte_array;
using namespace fetch::storage;

fetch::random::LaggedFibonacciGenerator<> lfg;

bool FailsBadLoad()
{
  {
    VersionedRandomAccessStack<FileBlockType<64>> stack;

    try
    {
      FileObject<VersionedRandomAccessStack<FileBlockType<64>>> file_object(stack);
      return false;
    }
    catch (const StorageException &e)
    {
      return true;
    }
  }
}

template <std::size_t BS>
bool BasicFileCreation()
{
  using stack_type = VersionedRandomAccessStack<FileBlockType<BS>>;
  stack_type stack;
  stack.Load("document_data.db", "doc_diff.db", true);

  FileObject<stack_type> file_object(stack);
  ByteArray              str;

  str.Resize(1 + (lfg() % 20000));
  for (std::size_t j = 0; j < str.size(); ++j)
  {
    str[j] = uint8_t(lfg() >> 9);
  }

  file_object.Write(str.pointer(), str.size());

  FileObject<stack_type> file_object2(stack, file_object.id());
  assert(file_object.id() == file_object2.id());

  ByteArray str2;
  str2.Resize(str.size());
  file_object2.Read(str2.pointer(), str2.size());

  if (file_object2.size() != str.size())
  {

    return false;
  }

  if (str != str2)
  {
    return false;
  }
  return true;
}

template <std::size_t BS>
bool MultipleFileCreation()
{
  using stack_type = VersionedRandomAccessStack<FileBlockType<BS>>;
  {
    stack_type stack;
    stack.New("document_data.db", "doc_diff.db");
  }

  for (std::size_t i = 0; i < 10; ++i)
  {
    if (!BasicFileCreation<BS>())
    {
      return false;
    }
  }
  return true;
}

template <std::size_t BS>
bool Overwriting()
{
  using stack_type = VersionedRandomAccessStack<FileBlockType<BS>>;
  stack_type stack;
  stack.New("document_data.db", "doc_diff.db");

  FileObject<stack_type> file_object(stack);
  ByteArray              str("Hello world! This is a great world."), f("Fetch"), ret;
  file_object.Seek(0);
  file_object.Write(str.pointer(), str.size());
  file_object.Seek(6);
  file_object.Write(f.pointer(), f.size());
  file_object.Write(f.pointer(), f.size());
  file_object.Seek(6);
  file_object.Write(str.pointer(), str.size());
  file_object.Write(f.pointer(), f.size());
  file_object.Write(f.pointer(), f.size());

  file_object.Seek(0);

  ret.Resize(file_object.size());
  file_object.Read(ret.pointer(), ret.size());

  return (ret.size() == file_object.size()) &&
         (ret == "Hello Hello world! This is a great world.FetchFetch");
}

template <std::size_t BS>
bool HashConsistency()
{
  using stack_type = VersionedRandomAccessStack<FileBlockType<BS>>;
  stack_type stack;
  stack.New("document_data.db", "doc_diff.db");

  FileObject<stack_type> file_object(stack);
  ByteArray              str;

  str.Resize(1 + (lfg() % 20000));
  for (std::size_t j = 0; j < str.size(); ++j)
  {
    str[j] = uint8_t(lfg() >> 9);
  }

  ByteArray ret;
  file_object.Write(str.pointer(), str.size());

  ret.Resize(file_object.size());
  file_object.Seek(0);
  file_object.Read(ret.pointer(), ret.size());

  return (str == ret) && (file_object.Hash() == crypto::Hash<crypto::SHA256>(str));
}

template <std::size_t BS>
bool FileLoadValueConsistency()
{
  using stack_type = VersionedRandomAccessStack<FileBlockType<BS>>;
  std::vector<ByteArray> values;
  std::vector<uint64_t>  file_ids;

  {
    stack_type stack;
    stack.New("document_data.db", "doc_diffXX.db");

    for (std::size_t n = 0; n < 100; ++n)
    {

      FileObject<stack_type> file_object(stack);
      ByteArray              str;

      str.Resize(1 + (lfg() % 2000));
      for (std::size_t j = 0; j < str.size(); ++j)
      {
        str[j] = uint8_t(lfg() >> 9);
      }

      file_object.Write(str.pointer(), str.size());
      file_ids.push_back(file_object.id());
      values.push_back(str);
    }

    for (std::size_t n = 0; n < 100; ++n)
    {
      FileObject<stack_type> file_object(stack, file_ids[n]);
      file_object.Seek(0);

      ByteArray test;

      test.Resize(file_object.size());
      file_object.Read(test.pointer(), test.size());

      if (test != values[n])
      {
        return false;
      }
    }
  }

  {
    stack_type stack;
    stack.Load("document_data.db", "doc_diffXX.db");

    for (std::size_t n = 0; n < 100; ++n)
    {
      FileObject<stack_type> file_object(stack, file_ids[n]);
      file_object.Seek(0);
      ByteArray test;

      test.Resize(file_object.size());
      file_object.Read(test.pointer(), test.size());

      if (test != values[n])
      {
        return false;
      }
    }
  }

  return true;
}

template <std::size_t BS, std::size_t FS>
bool FileSaveLoadFixedSize()
{
  using stack_type = RandomAccessStack<FileBlockType<BS>>;
  std::vector<ByteArray> strings;
  std::vector<uint64_t>  file_ids;

  {
    stack_type stack;
    stack.New("document_data.db");

    FileObject<stack_type> file_object(stack);
    ByteArray              str;

    str.Resize(1 + (lfg() % 2000));
    for (std::size_t j = 0; j < str.size(); ++j)
    {
      str[j] = uint8_t(lfg() >> 9);
    }

    strings.push_back(str);

    file_object.Write(str.pointer(), str.size());
    file_ids.push_back(file_object.id());
  }

  {

    stack_type stack;
    stack.Load("document_data.db");

    FileObject<stack_type> file_object(stack, file_ids[0]);
    file_object.Seek(0);
    ByteArray arr;
    arr.Resize(file_object.size());
    file_object.Read(arr.pointer(), arr.size());
    if (arr != strings[0])
    {
      return false;
    }
  }

  return true;
}

template <std::size_t BS>
bool FileLoadHashConsistency()
{
  using stack_type = RandomAccessStack<FileBlockType<BS>>;
  std::vector<ByteArray> strings;
  std::vector<ByteArray> hashes;
  std::vector<uint64_t>  file_ids;

  {
    stack_type stack;
    stack.New("document_data.db");  //,"doc_diffXX.db");

    for (std::size_t n = 0; n < 100; ++n)
    {

      FileObject<stack_type> file_object(stack);
      ByteArray              str;

      str.Resize(1 + (lfg() % 2000));
      for (std::size_t j = 0; j < str.size(); ++j)
      {
        str[j] = uint8_t(lfg() >> 9);
      }

      strings.push_back(str);

      file_object.Write(str.pointer(), str.size());
      file_ids.push_back(file_object.id());
      hashes.push_back(crypto::Hash<crypto::SHA256>(str));

      file_object.Seek(0);

      if (file_object.Hash() != hashes[n])
      {

        return false;
      }
    }
  }

  {
    stack_type stack;
    stack.Load("document_data.db");  //,"doc_diffXX.db");

    for (std::size_t n = 0; n < 100; ++n)
    {
      FileObject<stack_type> file_object(stack, file_ids[n]);
      file_object.Seek(0);

      if (file_object.Hash() != hashes[n])
      {
        return false;
      }
    }
  }

  return true;
}

// TEST(file_object, basic_functionality)
//{
//  //ASSERT_TRUE(FailsBadLoad());
//}

TEST(file_object, BasicFileCreation)
{
  ASSERT_TRUE(BasicFileCreation<2>());
}

TEST(file_object, FileSaveLoadFixedSize)
{
  ASSERT_TRUE((FileSaveLoadFixedSize<1, 1>()));
  ASSERT_TRUE((FileSaveLoadFixedSize<2, 1>()));
  ASSERT_TRUE((FileSaveLoadFixedSize<4, 1>()));
  ASSERT_TRUE((FileSaveLoadFixedSize<1, 0>()));
  ASSERT_TRUE((FileSaveLoadFixedSize<2, 0>()));
  ASSERT_TRUE((FileSaveLoadFixedSize<4, 0>()));
}

TEST(file_object, MultipleFileCreation)
{
  ASSERT_TRUE(MultipleFileCreation<1>());
  ASSERT_TRUE(MultipleFileCreation<2>());
  ASSERT_TRUE(MultipleFileCreation<4>());
  ASSERT_TRUE(MultipleFileCreation<9>());
  ASSERT_TRUE(MultipleFileCreation<1023>());
}

TEST(file_object, Overwriting)
{
  ASSERT_TRUE(Overwriting<1>());
  ASSERT_TRUE(Overwriting<2>());
  ASSERT_TRUE(Overwriting<4>());
  ASSERT_TRUE(Overwriting<7>());
  ASSERT_TRUE(Overwriting<2048>());
}

TEST(file_object, FileLoadValueConsistency)
{
  ASSERT_TRUE(FileLoadValueConsistency<1>());
  ASSERT_TRUE(FileLoadValueConsistency<2>());
  ASSERT_TRUE(FileLoadValueConsistency<4>());
  ASSERT_TRUE(FileLoadValueConsistency<7>());
}

TEST(file_object, hash_consistency)
{
  ASSERT_TRUE(HashConsistency<1>());
  ASSERT_TRUE(HashConsistency<2>());
  ASSERT_TRUE(HashConsistency<4>());
  ASSERT_TRUE(HashConsistency<9>());
  ASSERT_TRUE(HashConsistency<13>());
  ASSERT_TRUE(HashConsistency<1024>());
}

TEST(file_object, file_load_hash_consistency)
{
  ASSERT_TRUE(FileLoadHashConsistency<1>());
  ASSERT_TRUE(FileLoadHashConsistency<2>());
  ASSERT_TRUE(FileLoadHashConsistency<4>());
  ASSERT_TRUE(FileLoadHashConsistency<7>());
  ASSERT_TRUE(FileLoadHashConsistency<1023>());
}
