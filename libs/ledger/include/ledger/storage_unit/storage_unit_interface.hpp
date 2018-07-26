#pragma once

#include "core/byte_array/byte_array.hpp"
#include "storage/document.hpp"
#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace ledger {

class StateInterface {
public:
  using document_type = storage::Document;

  /// @name State Interface
  /// @{
  virtual document_type Get(byte_array::ConstByteArray const &key) = 0;
  virtual document_type GetOrCreate(byte_array::ConstByteArray const &key) = 0;
  virtual void Set(byte_array::ConstByteArray const &key, byte_array::ConstByteArray const& value) = 0;

  virtual bool Lock(byte_array::ConstByteArray const &key) = 0;
  virtual bool Unlock(byte_array::ConstByteArray const &key) = 0;
  /// @}
};

class StorageUnitInterface : public StateInterface
{
public:
  using hash_type = byte_array::ConstByteArray;
  using bookmark_type = uint64_t; // TODO: From keyvalue index

  // Construction / Destruction
  StorageUnitInterface() = default;
  virtual ~StorageUnitInterface() = default;

  /// @name Transaction Interface
  /// @{
  virtual void AddTransaction(chain::Transaction const &tx) = 0;
  virtual bool GetTransaction(byte_array::ConstByteArray const &digest, chain::Transaction &tx) = 0;
  /// @}

  /// @name Revertible Document Store Interface
  /// @{  
  virtual hash_type Hash() = 0;
  virtual void Commit(bookmark_type const &bookmark) = 0;
  virtual void Revert(bookmark_type const &bookmark) = 0;
  /// @}  
};

} // namespace ledger
} // namespace fetch



