#ifndef LEDGER_STORAGE_UNIT_INTERFACE_HPP
#define LEDGER_STORAGE_UNIT_INTERFACE_HPP
namespace fetch
{
namespace ledger
{

class StorageUnitInterface 
{
public:
  using resource_id_type = storage::ResourceID;
  using document_type = storage::Document;

  using hash_type = byte_array::ConstByteArray;
  typedef uint64_t bookmark_type; // TODO: From keyvalue index

  StorageUnitInterface() = default;
  virtual ~StorageUnitInterface() = default;

  /// @name Document Store Interface
  /// @{
  virtual document_type GetOrCreate(resource_id_type const &rid) = 0;
  virtual document_type Get(resource_id_type const &rid) = 0;
  virtual void Set(resource_id_type const &rid, byte_array::ConstByteArray const& value) = 0;
  /// @}
  
  /// @name Reveritble Document Store Interface
  /// @{  
  virtual hash_type Hash() = 0;
  virtual bookmark_type Commit(bookmark_type const& b) = 0;
  virtual void Revert(bookmark_type const &b) = 0;
  /// @}  

};

}
}



#endif
