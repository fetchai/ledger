#pragma once
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

namespace fetch {
namespace chain {

class Transaction;

/**
 * The transaction serializer is one of the two methods for constructing a transaction object. This
 * is intended to the main way that transaction are built in the system. I.e. they are received
 * over the wire on a HTTP or similar interface.
 */
class TransactionSerializer
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using ByteArray      = byte_array::ByteArray;

  static constexpr char const *LOGGING_NAME = "TxSerializer";

  // Construction / Destruction
  TransactionSerializer() = default;
  explicit TransactionSerializer(ConstByteArray data);
  TransactionSerializer(TransactionSerializer const &) = delete;
  TransactionSerializer(TransactionSerializer &&)      = delete;
  ~TransactionSerializer()                             = default;

  ConstByteArray const &data() const
  {
    return serial_data_;
  }

  // Transaction Payload Serialisation
  static ByteArray SerializePayload(Transaction const &tx);

  /// @name  Serialisation / Deserialization
  /// @{
  bool Serialize(Transaction const &tx);
  bool Deserialize(Transaction &tx) const;

  // Operators (throw on error)
  TransactionSerializer &operator<<(Transaction const &tx);
  TransactionSerializer &operator>>(Transaction &tx);
  /// @}

  // Operators
  TransactionSerializer &operator=(TransactionSerializer const &) = delete;
  TransactionSerializer &operator=(TransactionSerializer &&) = delete;

private:
  ConstByteArray serial_data_;
};

}  // namespace chain
}  // namespace fetch
