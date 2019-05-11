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

#include <gtest/gtest.h>
#include <vector>

#include "storage/storage_exception.hpp"
#include "mock_file_object.hpp"

using namespace fetch;
using namespace fetch::byte_array;
using namespace fetch::storage;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::StrictMock;

using FileObjectM   = StrictMock<MockFileObject>;
using FileObjectPtr = std::unique_ptr<FileObjectM>;

class FileObjectTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    file_object_ = std::make_unique<FileObjectM>();
  }

  void TearDown() override
  {
  }

  FileObjectPtr file_object_;
};

TEST_F(FileObjectTests, InvalidOperationsThrow)
{
  // Invalid to try to use the file object before new or load
  EXPECT_THROW(file_object_->CreateNewFile(), StorageException);
  //EXPECT_THROW(file_object_->CreateNewFile(), StorageException);
  //
  std::cerr << "now do this" << std::endl; // DELETEME_NH
  file_object_->CreateNewFile();
}

TEST_F(FileObjectTests, CreateNewFile)
{
  file_object_->New("test");
}

TEST_F(FileObjectTests, CreateAndRetrieveFile)
{
  file_object_->New("test");

  file_object_->CreateNewFile();

  file_object_->Write("whoooo, hoo");

  file_object_->CreateNewFile();

  file_object_->Write("Second, here we comeeee!");

  auto argh = file_object_->AsDocument();

  file_object_->SeekFile(1);
  argh = file_object_->AsDocument();

  file_object_->SeekFile(0);
  argh = file_object_->AsDocument();

  FETCH_UNUSED(argh);
}

// TODO(HUT): rewrite this to be less bad
/*
template <std::size_t BS>
bool BasicFileCreation()
{
  using stack_type = VersionedRandomAccessStack<FileBlockType<BS>>;
  stack_type stack;
  stack.Load("document_data.db", "doc_diff.db", true);

  FileObject<stack_type> file_object{};
  ByteArray              str;

  str.Resize(1 + (lfg1() % 20000));
  for (std::size_t j = 0; j < str.size(); ++j)
  {
    str[j] = uint8_t(lfg1() >> 9);
  }

  //  auto start = std::chrono::high_resolution_clock::now();
  file_object.Write(str.pointer(), str.size());

  FileObject<stack_type> file_object2(stack, file_object.id());
  assert(file_object.id() == file_object2.id());

  ByteArray str2;
  str2.Resize(str.size());
  file_object2.Read(str2.pointer(), str2.size());

  //  auto end = std::chrono::high_resolution_clock::now();
  if (file_object2.size() != str.size())
  {

    return false;
  }

  //  std::chrono::duration<double> diff = end-start;
  //  std::cout << "Write speed: " << str2.size() / diff.count()  / 1e6 << "
  //  mb/s\n";

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

  str.Resize(1 + (lfg1() % 20000));
  for (std::size_t j = 0; j < str.size(); ++j)
  {
    str[j] = uint8_t(lfg1() >> 9);
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

      str.Resize(1 + (lfg1() % 2000));
      for (std::size_t j = 0; j < str.size(); ++j)
      {
        str[j] = uint8_t(lfg1() >> 9);
      }

      file_object.Write(str.pointer(), str.size());
      file_ids.push_back(file_object.id());
      values.push_back(str);
    }

    for (std::size_t n = 0; n < 100; ++n)
    {
      FileObject<stack_type> file_object(stack, file_ids[n]);
      file_object.Seek(0);
      //      std::cout << "Treating: " << n << " " << file_ids[n]  <<
      //      std::endl;

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
    stack.New("document_data.db");  //,"doc_diffXX.db");

    FileObject<stack_type> file_object(stack);
    ByteArray              str;

    str.Resize(1 + (lfg1() % 2000));
    for (std::size_t j = 0; j < str.size(); ++j)
    {
      str[j] = uint8_t(lfg1() >> 9);
    }

    strings.push_back(str);

    file_object.Write(str.pointer(), str.size());
    file_ids.push_back(file_object.id());
  }

  {

    stack_type stack;
    stack.Load("document_data.db");  //,"doc_diffXX.db");

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

      str.Resize(1 + (lfg1() % 2000));
      for (std::size_t j = 0; j < str.size(); ++j)
      {
        str[j] = uint8_t(lfg1() >> 9);
      }

      strings.push_back(str);

      file_object.Write(str.pointer(), str.size());
      file_ids.push_back(file_object.id());
      hashes.push_back(crypto::Hash<crypto::SHA256>(str));

      file_object.Seek(0);

      if (file_object.Hash() != hashes[n])
      {
        std::cout << "Failed??? " << strings[n].size() << " " << file_object.size() << std::endl;

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

TEST(storage_file_object_gtest, Creating_reading)
{
  EXPECT_TRUE(BasicFileCreation<2>());

  EXPECT_TRUE((FileSaveLoadFixedSize<1, 1>()));
  EXPECT_TRUE((FileSaveLoadFixedSize<2, 1>()));
  EXPECT_TRUE((FileSaveLoadFixedSize<4, 1>()));
  EXPECT_TRUE((FileSaveLoadFixedSize<1, 0>()));
  EXPECT_TRUE((FileSaveLoadFixedSize<2, 0>()));
  EXPECT_TRUE((FileSaveLoadFixedSize<4, 0>()));

  EXPECT_TRUE(MultipleFileCreation<1>());
  EXPECT_TRUE(MultipleFileCreation<2>());
  EXPECT_TRUE(MultipleFileCreation<4>());
  EXPECT_TRUE(MultipleFileCreation<9>());
  EXPECT_TRUE(MultipleFileCreation<1023>());

  EXPECT_TRUE(Overwriting<1>());
  EXPECT_TRUE(Overwriting<2>());
  EXPECT_TRUE(Overwriting<4>());
  EXPECT_TRUE(Overwriting<7>());
  EXPECT_TRUE(Overwriting<2048>());

  EXPECT_TRUE(FileLoadValueConsistency<1>());
  EXPECT_TRUE(FileLoadValueConsistency<2>());
  EXPECT_TRUE(FileLoadValueConsistency<4>());
  EXPECT_TRUE(FileLoadValueConsistency<7>());
  //      EXPECT_TRUE( FileLoadValueConsistency<1023>() );
}

TEST(storage_file_object_gtest, Hash_consistency)
{
  EXPECT_TRUE(HashConsistency<1>());
  EXPECT_TRUE(HashConsistency<2>());
  EXPECT_TRUE(HashConsistency<4>());
  EXPECT_TRUE(HashConsistency<9>());
  EXPECT_TRUE(HashConsistency<13>());
  EXPECT_TRUE(HashConsistency<1024>());

  EXPECT_TRUE(FileLoadHashConsistency<1>());
  EXPECT_TRUE(FileLoadHashConsistency<2>());
  EXPECT_TRUE(FileLoadHashConsistency<4>());
  EXPECT_TRUE(FileLoadHashConsistency<7>());
  EXPECT_TRUE(FileLoadHashConsistency<1023>());
}
*/
