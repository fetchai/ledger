#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP
#include "crypto/sha256.hpp"
#include "byte_array/const_byte_array.hpp"
#include "serializer/byte_array_buffer.hpp"
#include "logger.hpp"

bool initialized = false;

namespace fetch {

typedef uint16_t group_type;

namespace chain {

struct TransactionSummary {
  typedef byte_array::ConstByteArray digest_type;
  std::vector< uint32_t > groups;
  mutable digest_type transaction_hash;
};

template< typename T >
void Serialize( T & serializer, TransactionSummary const &b) {
  serializer <<  b.groups << b.transaction_hash;
  //fetch::logger.Info("groups size ser: ", b.groups.size());
}

template< typename T >
void Deserialize( T & serializer, TransactionSummary &b) {
  serializer >>  b.groups >> b.transaction_hash;
  //fetch::logger.Info("groups size deser: ", b.groups.size());
}


class Transaction {
public:
  typedef crypto::SHA256 hasher_type;
  typedef TransactionSummary::digest_type digest_type;
  typedef byte_array::ConstByteArray arguments_type; // TODO: json doc with native serialization

  enum {
    VERSION = 1
  } ;

  void UpdateDigest() const {
    LOG_STACK_TRACE_POINT;

    if(modified_ == true)
    {
      serializers::ByteArrayBuffer buf;
      buf << summary_.groups << signatures_ << contract_name_ << arguments_;
      hasher_type hash;
      hash.Reset();
      hash.Update( buf.data());
      hash.Final();
      summary_.transaction_hash = hash.digest();
      modified_ = false;
    }
  }

  bool operator==(const Transaction &rhs) const
  {
    UpdateDigest();
    rhs.UpdateDigest();
    return digest() == rhs.digest();
  }

  bool operator<(const Transaction &rhs) const
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
      modified_ = true;
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
    modified_ = true;
  }

  void set_contract_name(byte_array::ConstByteArray const& name)
  {
    contract_name_ = name;
    modified_ = true;
  }

  void set_arguments(byte_array::ConstByteArray const& args)
  {
    arguments_ = args;
    modified_ = true;
  }

  std::vector< uint32_t > const &groups() const {
    return summary_.groups;
  }

  std::vector< byte_array::ConstByteArray > const& signatures() const
  {
    return signatures_;
  }

  std::vector< byte_array::ConstByteArray > &signatures() 
  {
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


  Transaction(){}

  Transaction(Transaction const &rhs) = default;
//  Transaction(Transaction const &rhs)
//  {
//    if(initialized == true)
//      fetch::logger.Info("copy 1");
//
//    summary_         = rhs.summary();
//    modified_        = true;
//    signature_count_ = rhs.signature_count();
//    signatures_      = signatures();
//    arguments_       = rhs.arguments();
//    contract_name_   = rhs.contract_name();
//    data_            = rhs.data();
//  }

  Transaction(Transaction &&rhs)                 = default;


  Transaction &operator=(Transaction const &rhs) = default;
//  Transaction &operator=(Transaction const &rhs)
//  {
//    fetch::logger.Info("copy 2");
//
//    summary_         = rhs.summary();
//    modified_        = true;
//    signature_count_ = rhs.signature_count();
//    signatures_      = signatures();
//    arguments_       = rhs.arguments();
//    contract_name_   = rhs.contract_name();
//    data_            = rhs.data();
//    return *this;
//  }
  Transaction &operator=(Transaction&& rhs)      = default;

private:
  TransactionSummary summary_;
  mutable bool               modified_ = true;

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

  serializer << uint16_t(b.VERSION);

  serializer << b.summary_;

  serializer <<  uint32_t(b.signatures().size());

  for(auto &sig: b.signatures()) {
    serializer << sig;
  }

  serializer << b.contract_name();
  serializer << b.arguments();

  if(b.summary_.groups.size() != 5)
  {
    //fetch::logger.Info("Groups not 5 for serialize!");
    //fetch::logger.Info("is: ", b.summary_.groups.size());
  }
  else
  {
    //fetch::logger.Info("serializing groups: ", b.summary_.groups.size());
  }

//  // TODO: (`HUT`) : remove 
//  if(b.signatures().size() == 0)
//  {
//    fetch::logger.Info("serialize sig size is 0!");
//    exit(1);
//  }
//
//  // TODO: (`HUT`) : remove 
//  if(b.summary_.groups.size() == 0)
//  {
//    fetch::logger.Info("serialize groups size is 0!");
//    exit(1);
//  }

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
    b.signatures().push_back(sig);
  }

  byte_array::ByteArray contract_name;
  typename Transaction::arguments_type arguments;
  serializer >> contract_name >> arguments;

  b.set_contract_name(contract_name);
  b.set_arguments(arguments);

  if(b.summary_.groups.size() != 5)
  {
    //fetch::logger.Info("Groups not 5 for deserialize!");
    //fetch::logger.Info("is: ", b.summary_.groups.size());
  }
  else
  {
    //fetch::logger.Info("deserializing groups: ", b.summary_.groups.size());
  }

//  // TODO: (`HUT`) : remove 
//  if(b.signatures().size() == 0)
//  {
//    fetch::logger.Info("serialize sig size is 0!");
//    exit(1);
//  }
//
//  // TODO: (`HUT`) : remove 
//  if(b.summary_.groups.size() == 0)
//  {
//    fetch::logger.Info("serialize groups size is 0!");
//    exit(1);
//  }

}

};
};
#endif
