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

#include "ledger/chain/transaction_serializer.hpp"
#include "core/byte_array/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "crypto/sha256.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_encoding.hpp"
#include "meta/type_traits.hpp"
#include "vectorise/platform.hpp"

namespace fetch {
namespace ledger {
namespace {

using byte_array::ByteArray;
using byte_array::ConstByteArray;
using crypto::Identity;
using serializers::ByteArrayBuffer;

using TokenAmount  = Transaction::TokenAmount;
using ContractMode = Transaction::ContractMode;

const uint8_t MAGIC              = 0xA1;
const uint8_t VERSION            = 1u;
const int8_t  UNIT_MEGA          = -2;
const int8_t  UNIT_KILO          = -1;
const int8_t  UNIT_DEFAULT       = 0;
const int8_t  UNIT_MILLI         = 1;
const int8_t  UNIT_MICRO         = 2;
const int8_t  UNIT_NANO          = 3;
const int8_t  CONTRACT_PRESENT   = 1;
const int8_t  CHAIN_CODE_PRESENT = 2;
const int8_t  SYNERGETIC_PRESENT = 3;

uint8_t Map(ContractMode mode)
{
  uint8_t value{0};

  switch (mode)
  {
  case ContractMode::NOT_PRESENT:
    value = 0;
    break;
  case ContractMode::PRESENT:
    value = 1u;
    break;
  case ContractMode::CHAIN_CODE:
    value = 2u;
    break;
  case ContractMode::SYNERGETIC:
    value = 3u;
    break;
  }

  return value;
}

ConstByteArray Encode(Address const &address)
{
  return address.address();
}

template <typename T>
meta::IfIsInteger<T, ConstByteArray> Encode(T value)
{
  return detail::EncodeInteger(std::move(value));
}

ConstByteArray Encode(ConstByteArray const &value)
{
  auto const length = Encode(value.size());
  return length + value;
}

ConstByteArray Encode(BitVector const &bits)
{
  auto const *      raw_data   = reinterpret_cast<uint8_t const *>(bits.data().pointer());
  std::size_t const raw_length = bits.data().size() * sizeof(BitVector::Block);
  std::size_t const size_bytes = bits.size() >> 3u;
  std::size_t const offset     = (raw_length - size_bytes) + 1;

  // create and populate the array
  ByteArray array;
  array.Resize(size_bytes);

  for (std::size_t i = 0, j = raw_length - offset; i < size_bytes; ++i, --j)
  {
    array[i] = raw_data[j];
  }

  return {array};
}

ConstByteArray Encode(Identity const &identity)
{
  ByteArray buffer;
  buffer.Append(uint8_t{0x04}, identity.identifier());

  return {buffer};
}

void Decode(ByteArrayBuffer &buffer, Address &address)
{
  Address::RawAddress raw_address;
  buffer.ReadBytes(raw_address.data(), raw_address.size());

  address = Address{raw_address};
}

template <typename T>
meta::IfIsInteger<T, T> Decode(ByteArrayBuffer &buffer)
{
  return detail::DecodeInteger<T>(buffer);
}

template <typename T>
meta::IfIsInteger<T> Decode(ByteArrayBuffer &buffer, T &value)
{
  value = Decode<T>(buffer);
}

void Decode(ByteArrayBuffer &buffer, BitVector &bits)
{
  auto *            raw_data   = reinterpret_cast<uint8_t *>(bits.data().pointer());
  std::size_t const raw_length = bits.data().size() * sizeof(BitVector::Block);
  std::size_t const size_bytes = bits.size() >> 3u;
  std::size_t const offset     = (raw_length - size_bytes) + 1;

  // read the expected number of bytes from the stream
  ConstByteArray bytes;
  buffer.ReadByteArray(bytes, size_bytes);

  auto const &bytes_ref{bytes};
  for (std::size_t i = 0, j = raw_length - offset; i < size_bytes; ++i, --j)
  {
    raw_data[j] = bytes_ref[i];
  }
}

void Decode(ByteArrayBuffer &buffer, ConstByteArray &bytes)
{
  // read the contents of the bytes
  std::size_t const byte_length = Decode<std::size_t>(buffer);

  // extract the bytes from the buffer
  buffer.ReadByteArray(bytes, byte_length);
}

void Decode(ByteArrayBuffer &buffer, Identity &identity)
{
  // read the identifier
  uint8_t identifier{0};
  buffer.ReadBytes(&identifier, 1u);

  if (identifier != 0x04)
  {
    throw std::runtime_error("Unsupported signature scheme");
  }

  // extract the public key
  ConstByteArray public_key;
  buffer.ReadByteArray(public_key, 64u);

  // create the identity
  identity = Identity{std::move(public_key)};
}

}  // namespace

TransactionSerializer::TransactionSerializer(ConstByteArray data)
  : serial_data_{std::move(data)}
{}

ByteArray TransactionSerializer::SerializePayload(Transaction const &tx)
{
  std::size_t const num_transfers  = tx.transfers().size();
  std::size_t const num_signatures = tx.signatories().size();

  auto const contract_mode = tx.contract_mode();

  // make an estimate for the serial size of the transaction and reserve this amount of buffer
  // space
  std::size_t const estimated_transaction_size = (num_transfers * 64u) + (num_signatures * 128u) +
                                                 tx.data().size() + tx.action().size() + 256u;

  ByteArray buffer;
  buffer.Reserve(estimated_transaction_size);

  // determine how to signal the number of signatures
  assert(num_signatures >= 1);
  std::size_t const num_extra_signatures = (num_signatures >= 0x40u) ? (num_signatures - 0x40u) : 0;
  std::size_t const signalled_signatures = num_signatures - (num_extra_signatures + 1);

  bool const has_valid_from = tx.valid_from() != 0;

  // format the main transaction header. Note that the charge_unit_flag is always zero here
  uint8_t header0{0};
  header0 |= static_cast<uint8_t>(VERSION << 5u);
  header0 |= static_cast<uint8_t>((num_transfers ? 1u : 0) << 2u);
  header0 |= static_cast<uint8_t>(((num_transfers > 1u) ? 1u : 0) << 1u);
  header0 |= static_cast<uint8_t>(has_valid_from ? 1u : 0);
  buffer.Append(MAGIC, header0);

  uint8_t header1{0};

  uint8_t const contract_mode_field = static_cast<uint8_t>(Map(contract_mode) << 6u);

  header1 |= contract_mode_field;
  header1 |= static_cast<uint8_t>(signalled_signatures) & 0x3Fu;
  buffer.Append(header1);

  buffer.Append(Encode(tx.from()));

  if (num_transfers > 1u)
  {
    buffer.Append(Encode(num_transfers - 2u));
  }

  for (auto const &transfer : tx.transfers())
  {
    buffer.Append(Encode(transfer.to), Encode(transfer.amount));
  }

  if (has_valid_from)
  {
    buffer.Append(Encode(tx.valid_from()));
  }

  buffer.Append(Encode(tx.valid_until()));

  // TODO(private issue 885): Increase efficiency by signaling with the charge_unit_flag
  buffer.Append(Encode(tx.charge()), Encode(tx.charge_limit()));

  // handle the signalling of the contract mode
  if (ContractMode::NOT_PRESENT != contract_mode)
  {
    auto const &shard_mask      = tx.shard_mask();
    auto const  shard_mask_size = static_cast<uint32_t>(shard_mask.size());

    if (shard_mask_size <= 1)
    {
      // in this case we are either explicitly signalling a wildcard or implicitly because the shard
      // mask length is 1.
      buffer.Append(uint8_t{0x80});
    }
    else
    {
      assert(platform::IsLog2(shard_mask_size));
      auto const log2_shard_mask_size = platform::ToLog2(shard_mask_size);

      if (shard_mask_size < 8u)
      {
        // in this case the shard mask is small and therefore can be totally contained in the
        // contract header
        uint8_t contract_header = static_cast<uint8_t>(shard_mask(0) & 0xFu);

        // signal the bit to signal the the shard mask it 2 or 4 bits
        if (log2_shard_mask_size == 2)
        {
          contract_header |= static_cast<uint8_t>(0x10u);
        }

        buffer.Append(contract_header);
      }
      else
      {
        // this format of transaction essentially places a limit on the number of individual
        // resource lanes that can be signalled to 512
        assert(shard_mask_size <= 512);

        // signal the size of the following shard bytes
        uint8_t const contract_header =
            static_cast<uint8_t>(0x40u) | static_cast<uint8_t>((log2_shard_mask_size - 3) & 0x3Fu);

        // write the header and the corresponding bytes
        buffer.Append(contract_header, Encode(shard_mask));
      }
    }

    switch (tx.contract_mode())
    {
    case ContractMode::PRESENT:
      buffer.Append(Encode(tx.contract_digest()), Encode(tx.contract_address()));
      break;
    case ContractMode::CHAIN_CODE:
      buffer.Append(Encode(tx.chain_code()));
      break;
    case ContractMode::SYNERGETIC:
      buffer.Append(Encode(tx.contract_digest()));
    default:
      break;
    }

    // add the action and data to the buffer
    buffer.Append(Encode(tx.action()), Encode(tx.data()));
  }

  if (num_extra_signatures > 0)
  {
    buffer.Append(Encode(num_extra_signatures));
  }

  for (auto const &signatory : tx.signatories())
  {
    buffer.Append(Encode(signatory.identity));
  }

  return buffer;
}

bool TransactionSerializer::Serialize(Transaction const &tx)
{
  // serialize the actual buffer
  auto buffer = SerializePayload(tx);

  for (auto const &signatory : tx.signatories())
  {
    buffer.Append(Encode(signatory.signature));
  }

  // update the serial data
  serial_data_ = std::move(buffer);

  return true;
}

bool TransactionSerializer::Deserialize(Transaction &tx) const
{
  serializers::ByteArrayBuffer buffer{serial_data_};

  std::size_t const payload_start = buffer.tell();

  // read the initial fixed header
  uint8_t header[3] = {0};
  buffer.ReadBytes(header, 3);

  if (header[0] != MAGIC)
  {
    return false;
  }

  uint8_t const version                 = (header[1] >> 5u) & 0x7u;
  uint8_t const charge_unit_flag        = (header[1] >> 3u) & 0x1u;
  uint8_t const transfer_flag           = (header[1] >> 2u) & 0x1u;
  uint8_t const multiple_transfers_flag = (header[1] >> 1u) & 0x1u;
  uint8_t const valid_from_flag         = header[1] & 0x1u;

  uint8_t const contract_type          = (header[2] >> 6u) & 0x3u;
  uint8_t const signature_count_minus1 = header[2] & 0x3fu;

  if (version != VERSION)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Version mismatch");
    return false;
  }

  Decode(buffer, tx.from_);

  if (transfer_flag)
  {
    std::size_t transfer_count = 1;

    if (multiple_transfers_flag)
    {
      std::size_t const transfer_count_minus2 = Decode<std::size_t>(buffer);
      transfer_count                          = transfer_count_minus2 + 2u;
    }

    tx.transfers_.resize(transfer_count);
    for (std::size_t i = 0; i < transfer_count; ++i)
    {
      auto &transfer = tx.transfers_[i];

      Decode(buffer, transfer.to);
      Decode(buffer, transfer.amount);
    }
  }

  if (valid_from_flag)
  {
    Decode(buffer, tx.valid_from_);
  }

  Decode(buffer, tx.valid_until_);

  Decode(buffer, tx.charge_);
  if (charge_unit_flag)
  {
    int8_t charge_unit{0};
    Decode(buffer, charge_unit);

    switch (charge_unit)
    {
    case UNIT_MEGA:
      tx.charge_ *= 10000000000000000ull;
      break;
    case UNIT_KILO:
      tx.charge_ *= 10000000000000ull;
      break;
    case UNIT_DEFAULT:
      tx.charge_ *= 10000000000ull;
      break;
    case UNIT_MILLI:
      tx.charge_ *= 10000000ull;
      break;
    case UNIT_MICRO:
      tx.charge_ *= 10000ull;
      break;
    case UNIT_NANO:
      tx.charge_ *= 10ull;
      break;
    default:
      break;
    }
  }

  Decode(buffer, tx.charge_limit_);

  if (contract_type == 0)
  {
    tx.contract_mode_    = Transaction::ContractMode::NOT_PRESENT;
    tx.contract_address_ = Address{};
    tx.contract_digest_  = Address{};
    tx.chain_code_       = ConstByteArray{};
  }
  else
  {
    // read the contract header
    uint8_t contract_header{0};
    buffer.ReadBytes(&contract_header, 1);

    bool const wildcard_flag = contract_header & 0x80u;

    if (wildcard_flag)
    {
      tx.shard_mask_ = BitVector{};
    }
    else
    {
      bool const extended_shard_mask_flag = contract_header & 0x40u;

      if (!extended_shard_mask_flag)
      {
        bool const shard_is_4bits = contract_header & 0x10u;

        tx.shard_mask_.Resize(shard_is_4bits ? 4u : 2u);

        tx.shard_mask_.set(0, ((contract_header & 0x1u) > 0));
        tx.shard_mask_.set(1, ((contract_header & 0x2u) > 0));

        if (shard_is_4bits)
        {
          tx.shard_mask_.set(2, ((contract_header & 0x4u) > 0));
          tx.shard_mask_.set(3, ((contract_header & 0x8u) > 0));
        }
      }
      else
      {
        // calculate the
        std::size_t const shard_mask_length_bits =
            1u << (static_cast<std::size_t>(contract_header & 0x3fu) + 3u);

        // create the mask of the correct size and decode the value
        tx.shard_mask_.Resize(shard_mask_length_bits);
        Decode(buffer, tx.shard_mask_);
      }
    }

    if (CONTRACT_PRESENT == contract_type)
    {
      tx.contract_mode_ = Transaction::ContractMode::PRESENT;
      tx.chain_code_    = ConstByteArray{};

      Decode(buffer, tx.contract_digest_);
      Decode(buffer, tx.contract_address_);
    }
    else if (CHAIN_CODE_PRESENT == contract_type)
    {
      tx.contract_mode_    = Transaction::ContractMode::CHAIN_CODE;
      tx.contract_address_ = Address{};
      tx.contract_digest_  = Address{};

      Decode(buffer, tx.chain_code_);
    }
    else if (SYNERGETIC_PRESENT == contract_type)
    {
      tx.contract_mode_    = Transaction::ContractMode::SYNERGETIC;
      tx.chain_code_       = ConstByteArray{};
      tx.contract_address_ = Address{};

      Decode(buffer, tx.contract_digest_);
    }

    // extract the data and actions
    Decode(buffer, tx.action_);
    Decode(buffer, tx.data_);
  }

  // determine the number of signatures that are contained
  std::size_t num_signatures = signature_count_minus1 + 1u;
  if (signature_count_minus1 == 0x3fu)
  {
    num_signatures += Decode<std::size_t>(buffer);
  }

  // clear and allocate the number of identities that are contained in this transaction
  tx.signatories_.clear();
  tx.signatories_.resize(num_signatures);
  for (std::size_t i = 0; i < num_signatures; ++i)
  {
    auto &current = tx.signatories_[i];

    Decode(buffer, current.identity);

    // ensure address is kept in sync
    current.address = Address{current.identity};
  }

  // compute the payload position
  std::size_t const payload_end  = buffer.tell();
  std::size_t const payload_size = payload_end - payload_start;

  crypto::SHA256 hash_function{};
  hash_function.Update(buffer.data().SubArray(payload_start, payload_size));

  for (std::size_t i = 0; i < num_signatures; ++i)
  {
    Decode(buffer, tx.signatories_[i].signature);

    // update signatories
    hash_function.Update(tx.signatories_[i].signature);
  }

  // compute the hash function
  tx.digest_ = hash_function.Final();

  return true;
}

TransactionSerializer &TransactionSerializer::operator<<(Transaction const &tx)
{
  if (!Serialize(tx))
  {
    throw std::runtime_error("Unable to serialize transaction from input stream");
  }

  return *this;
}

TransactionSerializer &TransactionSerializer::operator>>(Transaction &tx)
{
  if (!Deserialize(tx))
  {
    throw std::runtime_error("Unable to deserialize transaction from input stream");
  }

  return *this;
}

}  // namespace ledger
}  // namespace fetch
