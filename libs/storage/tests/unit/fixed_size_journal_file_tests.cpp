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
#include "gtest/gtest.h"
#include "storage/fixed_size_journal.hpp"

#include <fstream>
#include <memory>
#include <random>
#include <string>

namespace {

using fetch::storage::FixedSizeJournalFile;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ByteArray;

using FixedSizeJournalFilePtr = std::unique_ptr<FixedSizeJournalFile>;

const uint64_t SECTOR_SIZE = 32;

class FixedSizeJournalFileTests : public ::testing::Test
{
protected:
  using Rng     = std::default_random_engine;
  using RngWord = Rng::result_type;

  void SetUp() override
  {
    // generate a random file name for the test
    filename_ = GenerateRandomFilename();

    // create a new version of the journal for testing
    journal_ = std::make_unique<FixedSizeJournalFile>(SECTOR_SIZE);
    ASSERT_TRUE(journal_->New(filename_));
  }

  std::string GenerateRandomFilename()
  {
    return "journal_" + std::to_string(rng_()) + ".db";
  }

  ConstByteArray GenerateRandomBytes(std::size_t size)
  {
    ByteArray data;
    data.Resize(size);

    std::size_t const num_words = size / sizeof(RngWord);

    // stage 1: use all the output values of the RNG in order to populate the buffer
    auto *data_as_words = reinterpret_cast<RngWord *>(data.pointer());
    for (std::size_t i = 0; i < num_words; ++i)
    {
      data_as_words[i] = rng_();
    }

    // stage 2: depending on the value of `size` there might be some remaining bytes left in the
    //          buffer to generate
    std::size_t const remaining_offset = size - (num_words * sizeof(RngWord));

    for (std::size_t i = remaining_offset; i < size; ++i)
    {
      data[i] = static_cast<uint8_t>(rng_() & 0xFFu);
    }

    return {data};
  }

  std::random_device rd_;
  Rng                rng_{rd_()};

  std::string             filename_;
  FixedSizeJournalFilePtr journal_;
};

TEST_F(FixedSizeJournalFileTests, NoAccessToDataThatIsNotPresent)
{
  ConstByteArray buffer{};
  ASSERT_FALSE(journal_->Get(0, buffer));
  ASSERT_FALSE(journal_->Get(1, buffer));
  ASSERT_FALSE(journal_->Get(2, buffer));
}

TEST_F(FixedSizeJournalFileTests, CheckBasicSet)
{
  auto data1 = GenerateRandomBytes(SECTOR_SIZE);
  auto data2 = GenerateRandomBytes(SECTOR_SIZE);
  auto data3 = GenerateRandomBytes(SECTOR_SIZE);

  ASSERT_TRUE(journal_->Set(0, data1));
  ASSERT_TRUE(journal_->Set(1, data2));
  ASSERT_TRUE(journal_->Set(2, data3));

  // check that we can't retrieve data that is not
  ConstByteArray buffer{};
  ASSERT_TRUE(journal_->Get(0, buffer));
  ASSERT_EQ(buffer, data1);

  ASSERT_TRUE(journal_->Get(1, buffer));
  ASSERT_EQ(buffer, data2);

  ASSERT_TRUE(journal_->Get(2, buffer));
  ASSERT_EQ(buffer, data3);
}

TEST_F(FixedSizeJournalFileTests, CheckLoadingOfData)
{
  auto data1 = GenerateRandomBytes(SECTOR_SIZE);
  auto data2 = GenerateRandomBytes(SECTOR_SIZE);
  auto data3 = GenerateRandomBytes(SECTOR_SIZE);

  ASSERT_TRUE(journal_->Set(0, data1));
  ASSERT_TRUE(journal_->Set(1, data2));
  ASSERT_TRUE(journal_->Set(2, data3));

  // tear down original file and load again. Order important here so that a full flush occurs
  // before we attempt to read the same file on disk
  journal_ = std::make_unique<FixedSizeJournalFile>(SECTOR_SIZE);
  ASSERT_TRUE(journal_->Load(filename_));

  ConstByteArray buffer{};
  ASSERT_TRUE(journal_->Get(0, buffer));
  ASSERT_EQ(buffer, data1);

  ASSERT_TRUE(journal_->Get(1, buffer));
  ASSERT_EQ(buffer, data2);

  ASSERT_TRUE(journal_->Get(2, buffer));
  ASSERT_EQ(buffer, data3);
}

TEST_F(FixedSizeJournalFileTests, CheckManualFlush)
{
  auto orig = GenerateRandomBytes(SECTOR_SIZE);

  ASSERT_TRUE(journal_->Set(0, orig));

  // force a flush on the journal so that we load the contents again
  ASSERT_TRUE(journal_->Flush());

  FixedSizeJournalFile alternative{SECTOR_SIZE};
  ASSERT_TRUE(alternative.Load(filename_));

  ConstByteArray buffer{};
  ASSERT_TRUE(journal_->Get(0, buffer));
  ASSERT_EQ(buffer, orig);
}

TEST_F(FixedSizeJournalFileTests, CheckLoadFailureOnIncorrectSectorSize)
{
  // force a flush on the journal so that we load the contents again
  ASSERT_TRUE(journal_->Flush());

  FixedSizeJournalFile alternative{SECTOR_SIZE / 2};
  ASSERT_FALSE(alternative.Load(filename_));
}

TEST_F(FixedSizeJournalFileTests, CheckLoadFailureOnIncorrectFileSize)
{
  // add data to the journal file
  ASSERT_TRUE(journal_->Set(0, GenerateRandomBytes(SECTOR_SIZE)));
  ASSERT_TRUE(journal_->Set(1, GenerateRandomBytes(SECTOR_SIZE)));
  ASSERT_TRUE(journal_->Set(2, GenerateRandomBytes(SECTOR_SIZE)));

  // intentionally do not flush the disk - simulating a inconsistent state on disk:
  // `journal_->Flush();` <- (would resolve this error)

  FixedSizeJournalFile alternative{SECTOR_SIZE};
  ASSERT_FALSE(alternative.Load(filename_));
}

TEST_F(FixedSizeJournalFileTests, TestLoadWhenFileDoesNotExist)
{
  FixedSizeJournalFile alternative{SECTOR_SIZE};

  ASSERT_TRUE(alternative.Load(GenerateRandomFilename()));
}

TEST_F(FixedSizeJournalFileTests, HandleLoadFromEmptyExistingFile)
{
  auto const filename = GenerateRandomFilename();

  // generate the empty file
  {
    std::fstream stream{};
    stream.open(filename.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
  }

  FixedSizeJournalFile alternative{SECTOR_SIZE};
  ASSERT_TRUE(alternative.Load(filename));
}

TEST_F(FixedSizeJournalFileTests, CheckVariableLengthStores)
{
  auto data1 = GenerateRandomBytes(SECTOR_SIZE - 1);
  auto data2 = GenerateRandomBytes(SECTOR_SIZE - 2);
  auto data3 = GenerateRandomBytes(SECTOR_SIZE - 3);

  ASSERT_TRUE(journal_->Set(0, data1));
  ASSERT_TRUE(journal_->Set(1, data2));
  ASSERT_TRUE(journal_->Set(2, data3));

  ConstByteArray buffer{};
  ASSERT_TRUE(journal_->Get(0, buffer));
  ASSERT_EQ(buffer, data1);

  ASSERT_TRUE(journal_->Get(1, buffer));
  ASSERT_EQ(buffer, data2);

  ASSERT_TRUE(journal_->Get(2, buffer));
  ASSERT_EQ(buffer, data3);
}

TEST_F(FixedSizeJournalFileTests, CheckErrorWhenStoringGreaterThanSectorSize)
{
  auto data1 = GenerateRandomBytes(SECTOR_SIZE + 1);

  ASSERT_FALSE(journal_->Set(0, data1));
}

TEST_F(FixedSizeJournalFileTests, CheckOutOfOrderSectorWrites)
{
  auto data0 = GenerateRandomBytes(SECTOR_SIZE);
  auto data1 = GenerateRandomBytes(SECTOR_SIZE);
  auto data2 = GenerateRandomBytes(SECTOR_SIZE);
  auto data3 = GenerateRandomBytes(SECTOR_SIZE);
  auto data4 = GenerateRandomBytes(SECTOR_SIZE);

  ASSERT_TRUE(journal_->Set(2, data2));
  ASSERT_TRUE(journal_->Set(0, data0));
  ASSERT_TRUE(journal_->Set(3, data3));
  ASSERT_TRUE(journal_->Set(1, data1));
  ASSERT_TRUE(journal_->Set(4, data4));

  ConstByteArray buffer{};
  ASSERT_TRUE(journal_->Get(0, buffer));
  ASSERT_EQ(buffer, data0);

  ASSERT_TRUE(journal_->Get(1, buffer));
  ASSERT_EQ(buffer, data1);

  ASSERT_TRUE(journal_->Get(2, buffer));
  ASSERT_EQ(buffer, data2);

  ASSERT_TRUE(journal_->Get(3, buffer));
  ASSERT_EQ(buffer, data3);

  ASSERT_TRUE(journal_->Get(4, buffer));
  ASSERT_EQ(buffer, data4);
}

TEST_F(FixedSizeJournalFileTests, CheckLoadOfZeroSectorFile)
{
  journal_ = std::make_unique<FixedSizeJournalFile>(SECTOR_SIZE);
  ASSERT_TRUE(journal_->Load(filename_));
}

}  // namespace
