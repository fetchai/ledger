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

#include "core/mutex.hpp"

#include <fstream>
#include <string>

namespace fetch {

namespace byte_array {

class ConstByteArray;

}  // namespace byte_array

namespace storage {

class FixedSizeJournalFile
{
public:
  // Construction / Destruction
  explicit FixedSizeJournalFile(uint64_t sector_size);
  FixedSizeJournalFile(FixedSizeJournalFile const &) = delete;
  FixedSizeJournalFile(FixedSizeJournalFile &&)      = delete;
  ~FixedSizeJournalFile();

  // Startup
  bool New(std::string const &filename);
  bool Load(std::string const &filename);

  // Data Access
  bool Get(uint64_t sector, byte_array::ConstByteArray &buffer) const;
  bool Set(uint64_t sector, byte_array::ConstByteArray const &buffer);
  bool Flush();

  // Operators
  FixedSizeJournalFile &operator=(FixedSizeJournalFile const &) = delete;
  FixedSizeJournalFile &operator=(FixedSizeJournalFile &&) = delete;

private:
  bool           Clear(std::string const &filename);
  bool           InternalFlush();
  std::streamoff CalculateSectorOffset(uint64_t sector) const;

  mutable Mutex        lock_;
  uint64_t const       max_sector_data_size_;  ///< The maximum data size for a sector
  uint64_t const       sector_size_;           ///< The actual size of the sector on disk
  uint64_t             total_sectors_{0};      ///< Keeps track of the current number of blocks
  mutable std::fstream stream_;
};

}  // namespace storage
}  // namespace fetch