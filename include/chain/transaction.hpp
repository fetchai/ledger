#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP
#include "crypto/sha256.hpp"
#include "byte_array/const_byte_array.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "logger.hpp"

namespace fetch {
namespace chain {

class Transaction {
public:
  typedef crypto::SHA256 hasher_type;
  typedef byte_array::ConstByteArray digest_type;  
  typedef byte_array::ConstByteArray arguments_type; // TODO: json doc with native serialization

  enum {
    VERSION = 1
  } ;
  
  
  void UpdateDigest() {
    LOG_STACK_TRACE_POINT;    
    serializers::ByteArrayBuffer buf;
    buf << groups_ << signatures_ << contract_name_ << arguments_;
    hasher_type hash;
    hash.Reset();
    hash.Update( buf.data());
    hash.Final();
    digest_ = hash.digest();
  }

  void PushGroup(byte_array::ConstByteArray const &res)
  {
    LOG_STACK_TRACE_POINT;
    union 
    {
      uint8_t bytes[2];
      uint16_t value;      
    } d;
    d.value = 0;
    
    switch(res.size()) {
    case 0:
      break;
    default:
    case 2:
      d.bytes[1] = res[1];
    case 1:         
      d.bytes[0] = res[0];
    };
    
    groups_.push_back(d.value);
  }

  void PushGroup(uint16_t const &res)
  {
    LOG_STACK_TRACE_POINT;
    groups_.push_back(res);
  }

  bool UsesGroup(uint16_t g, uint16_t m) const
  {
    --m;    
    g &= m;
    
    bool ret = false;
    for(auto const &gg: groups_) {
      ret |= ( g == (gg&m) );      
    }
    
    return ret;
  }
  
  void PushSignature(byte_array::ConstByteArray const& sig)
  {
    LOG_STACK_TRACE_POINT;
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
  
  std::vector< uint16_t > const &groups() const {
    return groups_;
  }
  
  std::vector< byte_array::ConstByteArray > const& signatures() const {
    return signatures_;
  }

  byte_array::ConstByteArray const& contract_name() const {
    return contract_name_;
  }

  arguments_type const &arguments() const { return arguments_; }
  digest_type const & digest() const { return digest_; }

  uint32_t groups_count() const
  {
    return groups_count_;
  }
  
  uint32_t signature_count() const
  {
    return  signature_count_;
  }
  
  byte_array::ConstByteArray data() const { return data_; };
  
private:
  uint32_t groups_count_, signature_count_;
  byte_array::ConstByteArray data_;
  

  std::vector< uint16_t > groups_;
  std::vector< byte_array::ConstByteArray > signatures_;
  byte_array::ConstByteArray contract_name_;

  arguments_type arguments_;
  digest_type digest_;

  
};


template< typename T >
void Serialize( T & serializer, Transaction const &b) {
  serializer << uint16_t(b.VERSION);
  
  serializer <<  uint32_t(b.groups().size());

  for(auto &res: b.groups()) {
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
  uint16_t version;  
  serializer >> version;
  
  uint32_t size;
  serializer >>  size;

  for(std::size_t i=0; i < size; ++i) {
    uint16_t res;
    serializer >> res;
    b.PushGroup(res);
  }

  serializer >>  size;
  for(std::size_t i=0; i < size; ++i) {
    byte_array::ByteArray sig;
    serializer >> sig;
    b.PushGroup(sig);
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
