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
 * The transaction serialiser is one of the two methods for constructing a transaction object. This
 * is intended to the main way that transaction are built in the system. I.e. they are received
 * over the wire on a HTTP or similar interface.
 */
class TransactionSerialiser
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using ByteArray      = byte_array::ByteArray;

  static constexpr char const *LOGGING_NAME = "TxSerialiser";

  // Construction / Destruction
  TransactionSerialiser() = default;
  explicit TransactionSerialiser(ConstByteArray data);
  TransactionSerialiser(TransactionSerialiser const &) = delete;
  TransactionSerialiser(TransactionSerialiser &&)      = delete;
  ~TransactionSerialiser()                             = default;

  ConstByteArray const &data() const
  {
    return serial_data_;
  }

  // Transaction Payload Serialisation
  static ByteArray SerialisePayload(Transaction const &tx);

  /// @name  Serialisation / Deserialization
  /// @{
  bool Serialise(Transaction const &tx);
  bool Deserialise(Transaction &tx) const;

  // Operators (throw on error)
  TransactionSerialiser &operator<<(Transaction const &tx);
  TransactionSerialiser &operator>>(Transaction &tx);
  /// @}

  // Operators
  TransactionSerialiser &operator=(TransactionSerialiser const &) = delete;
  TransactionSerialiser &operator=(TransactionSerialiser &&) = delete;

private:
  ConstByteArray serial_data_;
};

}  // namespace chain
}  // namespace fetch
