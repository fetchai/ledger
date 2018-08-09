#pragma once

#include "core/byte_array/byte_array.hpp"
#include "ledger/chain/transaction.hpp"
#include "storage/document.hpp"
#include "storage/resource_mapper.hpp"

namespace fetch {
namespace ledger {

class StorageInterface
{
public:
  using Document        = storage::Document;
  using ResourceAddress = storage::ResourceAddress;
  using StateValue      = byte_array::ConstByteArray;

  /// @name State Interface
  /// @{
  virtual Document Get(ResourceAddress const &key)                          = 0;
  virtual Document GetOrCreate(ResourceAddress const &key)                  = 0;
  virtual void     Set(ResourceAddress const &key, StateValue const &value) = 0;
  virtual bool     Lock(ResourceAddress const &key)                         = 0;
  virtual bool     Unlock(ResourceAddress const &key)                       = 0;
  /// @}
};

class StorageUnitInterface : public StorageInterface
{
public:
  using hash_type     = byte_array::ConstByteArray;
  using bookmark_type = uint64_t;  // TODO(EJF): From keyvalue index

  // Construction / Destruction
  StorageUnitInterface()          = default;
  virtual ~StorageUnitInterface() = default;

  /// @name Transaction Interface
  /// @{
  virtual void AddTransaction(chain::Transaction const &tx)                                     = 0;
  virtual bool GetTransaction(byte_array::ConstByteArray const &digest, chain::Transaction &tx) = 0;
  /// @}

  /// @name Revertible Document Store Interface
  /// @{
  virtual hash_type Hash()                                = 0;
  virtual void      Commit(bookmark_type const &bookmark) = 0;
  virtual void      Revert(bookmark_type const &bookmark) = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
