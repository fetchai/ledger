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

#include "core/random/lcg.hpp"
#include "crypto/hash.hpp"
#include "crypto/sha256.hpp"
#include "mock_file_object.hpp"
#include "storage/storage_exception.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

using namespace fetch;
using namespace fetch::byte_array;
using namespace fetch::storage;

using ::testing::StrictMock;
using fetch::random::LinearCongruentialGenerator;

using FileObjectM   = StrictMock<MockFileObject>;
using FileObjectPtr = std::unique_ptr<FileObjectM>;

class FileObjectTests : public ::testing::Test
{
protected:
  void SetUp() override
  {
    file_object_ = std::make_unique<FileObjectM>();
  }

  char NewChar()
  {
    auto a = char(rng_());
    return a == '\0' ? '0' : a;
  }

  std::string GetStringForTesting()
  {
    uint64_t    size_desired = (1 << 10) + (rng_() & 0xFF);
    std::string ret;
    ret.resize(size_desired);

    for (std::size_t i = 0; i < 1 << 10; ++i)
    {
      ret[i] = NewChar();
    }

    return ret;
  }

  void Reset()
  {
    file_object_ = std::make_unique<FileObjectM>();
    consistency_check_.clear();
  }

  FileObjectPtr               file_object_;
  LinearCongruentialGenerator rng_;
  std::vector<uint64_t>       consistency_check_;
};

TEST_F(FileObjectTests, InvalidOperationsThrow)
{
  // Invalid to try to use the file object before new or load
  EXPECT_THROW(file_object_->CreateNewFile(), StorageException);
}

TEST_F(FileObjectTests, CreateNewFile)
{
  file_object_->New("test");
}

TEST_F(FileObjectTests, CreateAndWriteFilesConfirmUniqueIDs)
{
  file_object_->New("test");

  std::vector<std::string>                  strings_to_set;
  std::unordered_map<uint64_t, std::string> file_ids;

  strings_to_set.emplace_back("whoooo, hoo");
  strings_to_set.emplace_back("");
  strings_to_set.emplace_back("1");

  for (std::size_t i = 0; i < 100; ++i)
  {
    strings_to_set.push_back(GetStringForTesting());
  }

  for (auto const &string_to_set : strings_to_set)
  {
    file_object_->CreateNewFile(string_to_set.size());

    ASSERT_EQ(file_object_->FileObjectSize(), string_to_set.size());
    file_object_->Write(string_to_set);

    file_ids[file_object_->id()] = string_to_set;

    consistency_check_.push_back(file_object_->id());
    ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
  }

  ASSERT_EQ(file_ids.size(), strings_to_set.size());
}

TEST_F(FileObjectTests, CreateAndWriteFilesConfirmRecovery)
{
  file_object_->New("test");

  std::vector<std::string>                  strings_to_set;
  std::unordered_map<uint64_t, std::string> file_ids;

  strings_to_set.emplace_back("whoooo, hoo");
  strings_to_set.emplace_back("");
  strings_to_set.emplace_back("1");

  for (std::size_t i = 0; i < 100; ++i)
  {
    strings_to_set.push_back(GetStringForTesting());
  }

  for (auto const &string_to_set : strings_to_set)
  {
    file_object_->CreateNewFile();
    file_object_->Resize(string_to_set.size());
    ASSERT_EQ(file_object_->FileObjectSize(), string_to_set.size());
    file_object_->Write(string_to_set);

    file_ids[file_object_->id()] = string_to_set;
    consistency_check_.push_back(file_object_->id());
    ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
  }

  ASSERT_EQ(file_ids.size(), strings_to_set.size());

  for (auto it = file_ids.begin(); it != file_ids.end(); it++)
  {
    file_object_->SeekFile(it->first);
    auto doc = file_object_->AsDocument();

    ASSERT_EQ(doc.failed, false);
    ASSERT_EQ(doc.was_created, false);

    ASSERT_EQ(std::string{doc.document}, it->second);
    ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
  }
}

TEST_F(FileObjectTests, ResizeAndWriteFiles)
{
  file_object_->New("test");

  std::vector<std::string>                  strings_to_set;
  std::unordered_map<uint64_t, std::string> file_ids;

  strings_to_set.emplace_back("whoooo, hoo");
  strings_to_set.emplace_back("");
  strings_to_set.emplace_back("1");

  for (std::size_t i = 0; i < 100; ++i)
  {
    strings_to_set.push_back(GetStringForTesting());
  }

  for (auto const &string_to_set : strings_to_set)
  {
    file_object_->CreateNewFile();
    file_object_->Resize(string_to_set.size());
    ASSERT_EQ(file_object_->FileObjectSize(), string_to_set.size());
    file_object_->Write(string_to_set);

    file_ids[file_object_->id()] = string_to_set;
    consistency_check_.push_back(file_object_->id());
    ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
  }

  ASSERT_EQ(file_ids.size(), strings_to_set.size());

  std::random_device rd;
  std::mt19937       g(rd());

  for (std::size_t i = 0; i < 10; ++i)
  {
    std::shuffle(consistency_check_.begin(), consistency_check_.end(), g);

    for (auto const &index : consistency_check_)
    {
      ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
      file_object_->SeekFile(index);
      ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);

      auto new_string = GetStringForTesting();

      file_object_->Resize(new_string.size());
      ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);

      file_object_->Write(new_string);
      file_ids[index] = new_string;

      ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);

      auto doc = file_object_->AsDocument();

      ASSERT_EQ(file_object_->FileObjectSize(), new_string.size());
      ASSERT_EQ(doc.failed, false);
      ASSERT_EQ(doc.was_created, false);
      ASSERT_EQ(std::string{doc.document}, new_string);

      ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
    }
  }
}

TEST_F(FileObjectTests, EraseFiles)
{
  file_object_->New("test");
  std::unordered_map<uint64_t, std::string> file_ids;

  for (std::size_t i = 0; i < 100; ++i)
  {
    auto new_string = GetStringForTesting();

    file_object_->CreateNewFile(new_string.size());
    file_ids[file_object_->id()] = new_string;
    consistency_check_.push_back(file_object_->id());

    // Erase elements half of the time
    if ((i % 2) != 0u)
    {
      std::swap(consistency_check_[rng_() % consistency_check_.size()],
                consistency_check_[consistency_check_.size() - 1]);
      file_object_->SeekFile(consistency_check_[consistency_check_.size() - 1]);
      file_object_->Erase();
      file_ids.erase(consistency_check_[consistency_check_.size() - 1]);
      consistency_check_.pop_back();
    }

    ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
  }
}

TEST_F(FileObjectTests, SeekAndTellFiles)
{
  file_object_->New("test");

  for (std::size_t i = 0; i < 100; ++i)
  {
    auto new_string = GetStringForTesting();
    file_object_->CreateNewFile(new_string.size());

    ASSERT_EQ(file_object_->Tell(), 0);
    file_object_->Write(new_string);

    ASSERT_EQ(std::string{file_object_->AsDocument().document}.size(), new_string.size());

    for (std::size_t j = 0; j < 10; ++j)
    {
      // Force first iteration to change from the start for easier debugging.
      uint64_t index_to_change = j == 0 ? 0 : rng_() % new_string.size();
      uint64_t length_of_chars = rng_() % (new_string.size() - index_to_change);

      std::string new_chars(length_of_chars, NewChar());

      file_object_->Seek(index_to_change);
      file_object_->Write(reinterpret_cast<uint8_t const *>(new_chars.c_str()), new_chars.size());

      for (std::size_t k = 0; k < length_of_chars; ++k)
      {
        new_string[index_to_change + k] = new_chars[0];
      }

      // Seek back to 0 to avoid comparing against a truncated string.
      file_object_->Seek(0);

      ASSERT_EQ(std::string{file_object_->AsDocument().document}, new_string);
      ASSERT_EQ(std::string{file_object_->AsDocument().document}.size(), new_string.size());
    }

    consistency_check_.push_back(file_object_->id());
    ASSERT_EQ(file_object_->VerifyConsistency(consistency_check_), true);
  }
}

TEST_F(FileObjectTests, HashFiles)
{
  file_object_->New("test");

  auto new_string = GetStringForTesting();
  file_object_->CreateNewFile(new_string.size());
  file_object_->Write(new_string);

  ASSERT_EQ(file_object_->Hash(), crypto::Hash<crypto::SHA256>(new_string));
}
