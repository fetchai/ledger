#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP
#include "crypto/sha256.hpp"
#include "byte_array/referenced_byte_array.hpp"
#include "serializer/byte_array_buffer.hpp"

namespace fetch {
namespace chain {

class Transaction {
public:
  typedef crypto::SHA256 hasher_type;
  typedef byte_array::ConstByteArray digest_type;  
  typedef byte_array::ConstByteArray arguments_type; // TODO: json doc with native serialization

  
  void UpdateDigest() {
    serializers::ByteArrayBuffer buf;
    buf << resources_ << signatures_ << contract_name_ << arguments_;
    hasher_type hash;
    hash.Reset();
    hash.Update( buf.data());
    hash.Final();
    digest_ = hash.digest();
  }

  void PushResource(byte_array::ConstByteArray const &res)
  {
    resources_.push_back(res);
  }
  
  void PushSignature(byte_array::ConstByteArray const& sig)
  {
    signatures_.push_back(sig);
  }  

  void set_contract_name(byte_array::ConstByteArray const& name)
  {
    contract_name_ = name;
  }  
  
  void set_arguments(byte_array::ConstByteArray const& args)
  {
    arguments_ = args;
  }  

  
  std::vector< byte_array::ConstByteArray > const &resources() const {
    return resources_;
  }
  
  std::vector< byte_array::ConstByteArray > const& signatures() const {
    return signatures_;
  }

  byte_array::ConstByteArray const& contract_name() const {
    return contract_name_;
  }

  arguments_type const &arguments() const { return arguments_; }

  digest_type const & digest() const { return digest_; }
private:
  std::vector< byte_array::ConstByteArray > resources_;
  std::vector< byte_array::ConstByteArray > signatures_;
  byte_array::ConstByteArray contract_name_;

  arguments_type arguments_;
  digest_type digest_;

  
};


template< typename T >
void Serialize( T & serializer, Transaction const &b) {
  serializer <<  uint32_t(b.resources().size());
  for(auto &res: b.resources()) {
    serializer << res;
  }
  
  serializer <<  uint32_t(b.signatures().size());
  for(auto &sig: b.signatures()) {
    serializer << sig;
  }
  
  serializer << b.contract_name();
  serializer << b.arguments();  
}

template< typename T >
void Deserialize( T & serializer, Transaction &b) {
  uint32_t size;
  serializer >>  size;
  for(std::size_t i=0; i < size; ++i) {
    byte_array::ByteArray res;
    serializer >> res;
    b.PushResource(res);
  }

  serializer >>  size;
  for(std::size_t i=0; i < size; ++i) {
    byte_array::ByteArray sig;
    serializer >> sig;
    b.PushResource(sig);
  }
  
  byte_array::ByteArray contract_name;
  typename Transaction::arguments_type arguments;
  serializer >> contract_name >> arguments;

  b.set_contract_name(contract_name);
  b.set_arguments(arguments);
}

};

};
#endif
