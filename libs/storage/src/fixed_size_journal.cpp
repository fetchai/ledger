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

#include "storage/fixed_size_journal.hpp"

#include "core/byte_array/byte_array.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "logging/logging.hpp"
#include "vectorise/platform.hpp"

#include <array>

namespace fetch {
namespace storage {
namespace {

constexpr std::size_t FILE_HEADER_SIZE   = 26;
constexpr std::size_t SECTOR_HEADER_SIZE = 9;
constexpr uint8_t     SYNC_BYTE          = 0xa1;
constexpr uint8_t     EXPECTED_VERSION   = 1;
constexpr char const *LOGGING_NAME       = "JournalFile";

struct FileHeader
{
  uint8_t  version{EXPECTED_VERSION};
  uint64_t sector_size{0};
  uint64_t num_sectors{0};
};

constexpr uint64_t FILE_MAGIC = 0x46657463682e6169ull;  // 'Fetch.ai'

/**
 * Helper: Safely decrement one value from the other
 *
 * @param value The reference value
 * @param decrement The value to be subtracted from the reference
 * @return The calculated value
 */
uint64_t SafeDecrement(uint64_t value, uint64_t decrement)
{
  if (decrement > value)
  {
    return 0u;
  }

  return value - decrement;
}

/**
 * Helper: Read a value from a big endian source to the specified value
 *
 * @tparam T The type of destination value
 * @param src The source buffer to be read
 * @param value The value to be updated
 */
template <typename T>
void ReadBigEndian(void const *src, T &value)
{
  value = platform::FromBigEndian(*reinterpret_cast<T const *>(src));
}

/**
 * Helper: Write a value with big endianness to the specified buffer
 *
 * @tparam T The type of the source value
 * @param dst The destination buffer
 * @param value The value to be written
 */
template <typename T>
void WriteBigEndian(void *dst, T const &value)
{
  *reinterpret_cast<T *>(dst) = platform::ToBigEndian(value);
}

/**
 * Helper: Read data from the specified stream
 *
 * @param stream The stream to read data from
 * @param buffer The buffer to be populated
 * @param length The length of the buffer
 * @return true if successful, otherwise false
 */
bool ReadFromStream(std::fstream &stream, void *buffer, std::size_t length)
{
  stream.read(reinterpret_cast<char *>(buffer), static_cast<std::streamsize>(length));

  if (stream.fail() || stream.bad())
  {
    stream.clear();  // reset stream flags
    return false;
  }

  return true;
}

/**
 * Helper: Write data to the specified stream
 *
 * @param stream The stream to be written to
 * @param buffer The buffer to be written to the stream
 * @param length The length of the buffer
 * @return true if successful, otherwise false
 */
bool WriteToStream(std::fstream &stream, void const *buffer, std::size_t length)
{
  stream.write(reinterpret_cast<char const *>(buffer), static_cast<std::streamsize>(length));

  if (stream.fail() || stream.bad())
  {
    stream.clear();
    return false;
  }

  return true;
}

bool ReadFileHeader(std::fstream &stream, FileHeader &header)
{
  static_assert(FILE_HEADER_SIZE == 26, "ReadFileHeader only supports 26 byte file header");
  static_assert(sizeof(FileHeader) == 24,
                "FileHeader size appears to have changed from when this implementation was made");

  std::array<uint8_t, FILE_HEADER_SIZE> buffer{};
  if (!ReadFromStream(stream, buffer.data(), buffer.size()))
  {
    return false;
  }

  // read and check the file magic
  uint64_t file_magic{0};
  ReadBigEndian(&buffer[0], file_magic);
  if (file_magic != FILE_MAGIC)
  {
    return false;
  }

  // read the rest of the fields
  ReadBigEndian(&buffer[8], header.version);       // (2)
  ReadBigEndian(&buffer[10], header.sector_size);  // (8)
  ReadBigEndian(&buffer[18], header.num_sectors);  // (8)

  return true;
}

bool WriteFileHeader(std::fstream &stream, FileHeader const &header)
{
  static_assert(FILE_HEADER_SIZE == 26, "WriteFileHeader only supports 26 byte file header");
  static_assert(sizeof(FileHeader) == 24,
                "FileHeader size appears to have changed from when this implementation was made");

  // populate the header buffer
  std::array<uint8_t, FILE_HEADER_SIZE> buffer{};
  WriteBigEndian(&buffer[0], FILE_MAGIC);           // (8)
  WriteBigEndian(&buffer[8], header.version);       // (2)
  WriteBigEndian(&buffer[10], header.sector_size);  // (8)
  WriteBigEndian(&buffer[18], header.num_sectors);  // (8)

  return WriteToStream(stream, buffer.data(), buffer.size());
}

/**
 * Read a sector header from the disk
 *
 * @param stream The file stream to be read
 * @param data_size The sector data length to be populated
 * @return true if successful, otherwise false
 */
bool ReadSectorHeader(std::fstream &stream, uint64_t &data_size)
{
  static_assert(SECTOR_HEADER_SIZE == 9, "ReadSectorHeader only supports 9 byte file header");
  static_assert(sizeof(data_size) == 8,
                "ReadSectorHeader payload appears to be a different size for the implementation");

  std::array<uint8_t, SECTOR_HEADER_SIZE> buffer{};
  if (!ReadFromStream(stream, buffer.data(), buffer.size()))
  {
    return false;
  }

  // read and check the sync byte
  uint8_t sync_byte{0};
  ReadBigEndian(&buffer[0], sync_byte);

  if (sync_byte != SYNC_BYTE)
  {
    return false;
  }

  // read the header size
  ReadBigEndian(&buffer[1], data_size);

  return true;
}

/**
 * Write the sector header to the stream
 *
 * @param stream The stream to be written to
 * @param data_size The size of the contents for this sector
 * @return true if successful, otherwise false
 */
bool WriteSectorHeader(std::fstream &stream, uint64_t data_size)
{
  static_assert(SECTOR_HEADER_SIZE == 9, "WriteSectorHeader only supports 9 byte file header");
  static_assert(sizeof(data_size) == 8,
                "WriteSectorHeader payload appears to be a different size for the implementation");

  // populate the buffer
  std::array<uint8_t, SECTOR_HEADER_SIZE> buffer{};

  WriteBigEndian(&buffer[0], SYNC_BYTE);
  WriteBigEndian(&buffer[1], data_size);

  return WriteToStream(stream, buffer.data(), buffer.size());
}

/**
 * Write the contents of the sector to the stream
 *
 * @param stream The stream to be written to
 * @param contents The contents to be written to the stream
 * @return true if successful, otherwise false
 */
bool WriteSectorContents(std::fstream &stream, byte_array::ConstByteArray const &contents)
{
  return WriteToStream(stream, contents.pointer(), contents.size());
}

}  // namespace

/**
 * Create a journal file with a specified sector size
 *
 * @param sector_size The specified sector size
 */
FixedSizeJournalFile::FixedSizeJournalFile(uint64_t sector_size)
  : max_sector_data_size_{sector_size}
  , sector_size_{max_sector_data_size_ + SECTOR_HEADER_SIZE}
{}

/**
 * Destruct the journal file
 */
FixedSizeJournalFile::~FixedSizeJournalFile()
{
  InternalFlush();
}

/**
 * Create a new journal file with the specified filename. If that filename already exists all
 * contents of the file is deleted
 *
 * @param filename The filename for the file
 * @return true if successful, otherwise false
 */
bool FixedSizeJournalFile::New(std::string const &filename)
{
  FETCH_LOCK(lock_);

  return Clear(filename);
}

/**
 * Load a journal file with the specified filename
 *
 * @param filename The filename for the file
 * @return true if successful, otherwise false
 */
bool FixedSizeJournalFile::Load(std::string const &filename)
{
  FETCH_LOCK(lock_);

  stream_.open(filename.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);

  bool const success = stream_.is_open() && !(stream_.bad() || stream_.fail());
  if (!success)
  {
    // if the load fails then we will not be able to load the contents, we therefore need to create
    // a new file.
    return Clear(filename);
  }

  // read the size of the file
  auto const file_size = static_cast<uint64_t>(stream_.tellp());

  // we are loading a empty file, in this way we act like making a new file
  if (file_size == 0)
  {
    return InternalFlush();
  }

  // if we get this far then we have opened an existing file. perform a basic sanity check on the
  // contents of the file so see if the number of entries that are present, match what is expected
  stream_.seekg(0);
  if (stream_.bad() || stream_.fail())
  {
    stream_.clear();  // reset stream flags
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to perform stream seek");
    return false;
  }

  // read the file header
  FileHeader header{};
  if (!ReadFileHeader(stream_, header))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to read file header");
    return false;
  }

  // check sector compatibility
  if (header.sector_size != sector_size_)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Sector size mismatch");
    return false;
  }

  // compute the expected file size based on the header information
  uint64_t const min_expected_file_size = FILE_HEADER_SIZE +
                                          (SafeDecrement(header.num_sectors, 1u) * sector_size_) +
                                          SECTOR_HEADER_SIZE;
  uint64_t const max_expected_file_size = FILE_HEADER_SIZE + (header.num_sectors * sector_size_);

  // final check make sure that the actual file size matches the one that is written to the header
  // file.
  bool const file_size_within_bounds =
      (file_size >= min_expected_file_size) && (file_size <= max_expected_file_size);
  if (!file_size_within_bounds)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "File size mismatch. Actual: ", file_size,
                   " expected range: ", min_expected_file_size, " - ", max_expected_file_size);
    return false;
  }

  return true;
}

/**
 * Read the contents of the sector specified
 *
 * @param sector The sector index to read
 * @param buffer The buffer to populate with the contents
 * @return true if successful, otherwise false
 */
bool FixedSizeJournalFile::Get(uint64_t sector, byte_array::ConstByteArray &buffer) const
{
  FETCH_LOCK(lock_);

  // advance to the sector and read the entire sector
  stream_.seekg(CalculateSectorOffset(sector));
  if (stream_.bad() || stream_.fail())
  {
    stream_.clear();  // reset stream flags
    FETCH_LOG_DEBUG(LOGGING_NAME, "Failed to perform stream seek");
    return false;
  }

  // read the sector header and determine the length of data stored here
  uint64_t payload_size{0};
  if (!ReadSectorHeader(stream_, payload_size))
  {
    return false;
  }

  // allocate the sector size to be read
  byte_array::ByteArray read_buffer;
  read_buffer.Resize(payload_size);

  if (!ReadFromStream(stream_, read_buffer.pointer(), read_buffer.size()))
  {
    return false;
  }

  // return the read sector
  buffer = byte_array::ConstByteArray{read_buffer};

  return true;
}

/**
 * Write the contents of a buffer into the specified sector
 *
 * @param sector The sector index on the disk
 * @param buffer The buffer of the data to store
 * @return true if successful, otherwise false
 */
bool FixedSizeJournalFile::Set(uint64_t sector, byte_array::ConstByteArray const &buffer)
{
  FETCH_LOCK(lock_);

  // ensure that we can save this much data
  if (buffer.size() > max_sector_data_size_)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Buffer too large for configured sector size");
    return false;
  }

  // advance to the sector and read the entire sector
  stream_.seekp(CalculateSectorOffset(sector));
  if (stream_.bad() || stream_.fail())
  {
    stream_.clear();  // reset stream flags
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to perform stream seek");
    return false;
  }

  // write the header
  if (!WriteSectorHeader(stream_, buffer.size()))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to write sector header");
    return false;
  }

  // write the contents
  if (!WriteSectorContents(stream_, buffer))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to write sector contents");
    return false;
  }

  // update the total sectors
  total_sectors_ = std::max(sector + 1, total_sectors_);

  return true;
}

/**
 * Flush pending updates to disk
 *
 * @return true if successful, otherwise false
 */
bool FixedSizeJournalFile::Flush()
{
  FETCH_LOCK(lock_);

  return InternalFlush();
}

/**
 * Internal: Delete the contents of the file and clean it down to zero again
 *
 * @note Thread safety is the prerogative of the caller
 *
 * @param filename The filename to be reset
 * @return true if successful, otherwise false
 */
bool FixedSizeJournalFile::Clear(std::string const &filename)
{
  stream_.open(filename.c_str(), std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

  // ensure that the total sectors is correctly reset too
  total_sectors_ = 0;

  // check to see if the open was successful
  bool const success = stream_.is_open() && !(stream_.bad() || stream_.fail());
  if (!success)
  {
    stream_.clear();  // reset stream flags
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to open requested file");
    return false;
  }

  // perform a flush, this will make sure that the header is correctly populated
  return InternalFlush();
}

/**
 * Internal: Flush pending updates to disk
 *
 * @return true if successful, otherwise false
 */
bool FixedSizeJournalFile::InternalFlush()
{
  // go back to the start of the file
  stream_.seekg(0);
  if (stream_.bad() || stream_.fail())
  {
    stream_.clear();  // reset stream flags
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to perform stream seek");
    return false;
  }

  // generate the file header
  FileHeader header{};
  header.sector_size = sector_size_;
  header.num_sectors = total_sectors_;
  header.version     = EXPECTED_VERSION;

  if (!WriteFileHeader(stream_, header))
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to write file header");
    return false;
  }

  // flush the stream
  stream_.flush();
  if (stream_.bad() || stream_.fail())
  {
    stream_.clear();  // reset stream flags
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to flush file header to the stream");
    return false;
  }

  return true;
}

/**
 * Calculate the disk sector offset for a specified sector
 *
 * @param sector The specified sector index
 * @return The disk offset
 */
std::streamoff FixedSizeJournalFile::CalculateSectorOffset(uint64_t sector) const
{
  return static_cast<std::streamoff>(FILE_HEADER_SIZE + (sector * sector_size_));
}

}  // namespace storage
}  // namespace fetch
