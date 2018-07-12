#ifndef CHAIN_MUTABLE_TRANSACTION_HPP
#define CHAIN_MUTABLE_TRANSACTION_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/referenced_byte_array.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/sha256.hpp"
#include "ledger/identifier.hpp"

#include<vector>

namespace fetch {
namespace chain {

 
struct TransactionSummary {
  typedef byte_array::ConstByteArray group_type;  
  typedef byte_array::ConstByteArray digest_type;

  std::vector<group_type> groups;
  digest_type             transaction_hash;
  uint64_t                fee{0};

  // TODO: (EJF) Remove but linked to optimisation
  std::size_t short_id;
};

template <typename T>
void Serialize(T &serializer, TransactionSummary const &b) {
  serializer << b.groups << b.fee << b.transaction_hash;
}

template <typename T>
void Deserialize(T &serializer, TransactionSummary &b) {
  serializer >> b.groups >> b.fee >> b.transaction_hash;
}

class MutableTransaction {
public:
  typedef crypto::SHA256                  hasher_type;
  typedef TransactionSummary::digest_type digest_type;
  typedef TransactionSummary::group_type group_type;  

  std::vector<group_type> const &groups() const {
    return summary_.groups;
  }

  TransactionSummary const &summary() const {
    return summary_;
  }

  byte_array::ConstByteArray const &data() const {
    return data_;
  }

  byte_array::ConstByteArray const &signature() const {
    return signature_;
  }

  //ledger::Identifier
  byte_array::ConstByteArray const &contract_name() const {
    return contract_name_;
  }

  digest_type const &digest() const {
    return summary_.transaction_hash;
  }
  
  
  void Copy(MutableTransaction const &rhs) {
    copy_on_write_ = true;
    
    summary_       = rhs.summary();
    data_          = rhs.data();
    signature_     = rhs.signature();    
    contract_name_ = rhs.contract_name_;
  }

  MutableTransaction() = default;
  MutableTransaction(MutableTransaction const &rhs) = delete;  
  MutableTransaction &operator=(MutableTransaction const &rhs) = delete;
  MutableTransaction(MutableTransaction &&rhs) = default;  
  MutableTransaction &operator=(MutableTransaction&& rhs) = default;
  
  enum { VERSION = 1 };

  void UpdateDigest() {
    LOG_STACK_TRACE_POINT;

    serializers::ByteArrayBuffer buf;
    buf << summary_.groups << signature_
      << contract_name_ << summary_.fee;
    hasher_type hash;
    hash.Reset();
    hash.Update(buf.data());
    hash.Final();
    summary_.transaction_hash = hash.digest();
  }

  bool Verify() 
  {
    return true;    
    // TODO_FAIL("Needs implementing");
  }
  
  void PushGroup(byte_array::ConstByteArray const &res) {
    LOG_STACK_TRACE_POINT;    
    if(copy_on_write_) Clone();    
    summary_.groups.push_back(res);
  }

  void set_summary(TransactionSummary const &summary) {
    if(copy_on_write_) Clone();    
    summary_ = summary;
  }

  void set_data(byte_array::ConstByteArray const &data) {
    if(copy_on_write_) Clone();    
    data_ = data;
  }

  void set_signature(byte_array::ConstByteArray const & sig) {
    if(copy_on_write_) Clone();    
    signature_ = sig;
  }

  void set_contract_name(byte_array::ConstByteArray const & name) {
    contract_name_ = name;
  }

protected:
  mutable bool copy_on_write_ = false;

  void Clone() {
    copy_on_write_ = false;
    summary_.transaction_hash = this->summary().transaction_hash.Copy();
    data_          = this->data().Copy();
    signature_     = this->signature().Copy();
  }
  
private:
  TransactionSummary         summary_;
  byte_array::ConstByteArray data_;
  byte_array::ConstByteArray signature_;
  byte_array::ConstByteArray contract_name_;
};



}
}
#endif
