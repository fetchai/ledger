#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP
#include "crypto/sha256.hpp"
#include "byte_array/const_byte_array.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "logger.hpp"

namespace fetch {
namespace chain {

struct TransactionSummary {
  typedef byte_array::ConstByteArray digest_type;    
  std::vector< uint32_t > groups;  
  mutable digest_type transaction_hash;
};

template< typename T >
void Serialize( T & serializer, TransactionSummary const &b) {
  serializer <<  b.groups << b.transaction_hash;  
}

template< typename T >
void Deserialize( T & serializer, TransactionSummary &b) {
  serializer >>  b.groups >> b.transaction_hash;  
}

  
class Transaction {
public:
  typedef crypto::SHA256 hasher_type;
  typedef TransactionSummary::digest_type digest_type; 
  typedef byte_array::ConstByteArray arguments_type; // TODO: json doc with native serialization

  enum {
    VERSION = 1
  } ;

  bool wasDestroyed = false;

  Transaction()
  {
    {
      std::stringstream stream;
      stream << "Create transaction" << std::endl;
      std::cerr << stream.str();
    }
  }

  //Transaction(Transaction const &rhs)            = delete;
  //Transaction(Transaction &&rhs)                 = default;
  //Transaction &operator=(Transaction const &rhs) = delete;
  //Transaction &operator=(Transaction&& rhs)      = default;

  ~Transaction()
  {
    {
      std::stringstream stream;
      stream << "Destroy transaction " << byte_array::ToHex(summary().transaction_hash) << std::endl;
      std::cerr << stream.str();
    }
    if(wasDestroyed)
    {
      {
        std::stringstream stream;
        stream << "trying to double destroy transaction!" << std::endl;
        std::cerr << stream.str();
      }
    }
    wasDestroyed = true;
  }
  
  
  void UpdateDigest() const {
    LOG_STACK_TRACE_POINT;    

    if(modified == true)
    {
      serializers::ByteArrayBuffer buf;
      buf << summary_.groups << signatures_ << contract_name_ << arguments_;
      hasher_type hash;
      hash.Reset();
      hash.Update( buf.data());
      hash.Final();
      summary_.transaction_hash = hash.digest();
      modified = false;
    }
  }

  template<typename T>
  bool operator==(T&& rhs) const
  {
    UpdateDigest();
    rhs.UpdateDigest();
    return digest() == rhs.digest();
  }

  template<typename T>
  bool operator< (T&& rhs) const
  {
    UpdateDigest();
    rhs.UpdateDigest();
    return digest() < rhs.digest();
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
      /*
TODO: Make 32 bit compat     
    case 4:
      d.bytes[3] = res[3];      
    case 3:
      d.bytes[2] = res[2];      
*/      
    case 2:
      d.bytes[1] = res[1];
    case 1:         
      d.bytes[0] = res[0];
    };
//    std::cout << byte_array::ToHex( res) << " >> " << d.value << std::endl;
    
//    assert(d.value < 10);
    
    PushGroup(d.value);    
    modified = true;
  }

  void PushGroup(uint32_t const &res)
  {
    LOG_STACK_TRACE_POINT;
    bool add = true;
    for(auto &g: summary_.groups) {
      if(g == res) {
        add = false;
        break;
      }
      
    }
    
    if(add)
    {
      summary_.groups.push_back(res);
      modified = true;
    }
    
  }

  bool UsesGroup(uint16_t g, uint16_t m) const
  {
    --m;    
    g &= m;
    
    bool ret = false;
    for(auto const &gg: summary_.groups) {
      ret |= ( g == (gg&m) );      
    }
    
    return ret;
  }
  
  void PushSignature(byte_array::ConstByteArray const& sig)
  {
    LOG_STACK_TRACE_POINT;
    signatures_.push_back(sig);
    modified = true;
  }  

  void set_contract_name(byte_array::ConstByteArray const& name)
  {
    contract_name_ = name;
    modified = true;
  }  
  
  void set_arguments(byte_array::ConstByteArray const& args)
  {
    arguments_ = args;
    modified = true;
  }  
  
  std::vector< uint32_t > const &groups() const {
    return summary_.groups;
  }
  
  std::vector< byte_array::ConstByteArray > const& signatures() const {
    return signatures_;
  }

  byte_array::ConstByteArray const& contract_name() const {
    return contract_name_;
  }

  arguments_type const &arguments() const { return arguments_; }
  digest_type const & digest() const { UpdateDigest(); return summary_.transaction_hash; }

  uint32_t signature_count() const
  {
    return  signature_count_;
  }
  
  byte_array::ConstByteArray data() const { return data_; };

  TransactionSummary const & summary() const { UpdateDigest(); return summary_; }
private:
  TransactionSummary summary_;
  mutable bool               modified = true;
  
  uint32_t  signature_count_;
  byte_array::ConstByteArray data_;
  // TODO: Add resources
  std::vector< byte_array::ConstByteArray > signatures_;
  byte_array::ConstByteArray contract_name_;

  arguments_type arguments_;

  template< typename T >
  friend void Serialize(T&, Transaction const&);
  template< typename T >
  friend void Deserialize(T&, Transaction &);  
};


template< typename T >
void Serialize( T & serializer, Transaction const &b) {

  {
    std::stringstream stream;
    stream << "serializing tranaction " << b.wasDestroyed << std::endl;
    std::cerr << stream.str();
  }

  serializer << uint16_t(b.VERSION);

  {
    std::stringstream stream;
    stream << "serializing tranaction1 " << b.wasDestroyed << std::endl;
    std::cerr << stream.str();
  }
  
  serializer << b.summary_;
  {
    std::stringstream stream;
    stream << "serializing tranaction2 " << b.wasDestroyed << std::endl;
    std::cerr << stream.str();
  }

  serializer <<  uint32_t(b.signatures().size());
  {
    std::stringstream stream;
    stream << "serializing tranaction3 " << b.wasDestroyed << std::endl;
    std::cerr << stream.str();
  }
  for(auto &sig: b.signatures()) {
    serializer << sig;
  }
  {
    std::stringstream stream;
    stream << "serializing tranaction4 " << b.wasDestroyed << std::endl;
    std::cerr << stream.str();
  }

  serializer << b.contract_name();  
  {
    std::stringstream stream;
    stream << "serializing tranaction5 " << b.wasDestroyed << std::endl;
    std::cerr << stream.str();
  }
  serializer << b.arguments();  
  {
    std::stringstream stream;
    stream << "serializing tranaction6 " << b.wasDestroyed << std::endl;
    std::cerr << stream.str();
  }
}

template< typename T >
void Deserialize( T & serializer, Transaction &b) {
  uint16_t version;  
  serializer >> version;

  serializer >> b.summary_;

  uint32_t size;
  serializer >> size;
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
