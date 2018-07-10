#ifndef CHAIN_BASIC_TRANSACTION_HPP
#define CHAIN_BASIC_TRANSACTION_HPP

#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/referenced_byte_array.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/sha256.hpp"
#include "ledger/identifier.hpp"

namespace fetch {
typedef byte_array::ConstByteArray group_type;

namespace chain {

struct TransactionSummary {
  typedef byte_array::ConstByteArray digest_type;

  std::vector<group_type> groups;
  digest_type             transaction_hash;
  uint64_t                fee{0};
  uint64_t                short_id;
};

template <typename T>
void Serialize(T &serializer, TransactionSummary const &b) {
  serializer << b.groups << b.fee << b.transaction_hash;
}

template <typename T>
void Deserialize(T &serializer, TransactionSummary &b) {
  serializer >> b.groups >> b.fee >> b.transaction_hash;
}

class BasicTransaction {
 public:
  typedef crypto::SHA256                  hasher_type;
  typedef TransactionSummary::digest_type digest_type;

  BasicTransaction() {}

  BasicTransaction(BasicTransaction const &rhs)
  {
    summary_ = rhs.summary();
    summary_.transaction_hash = rhs.summary().transaction_hash.Copy();

    data_          = rhs.data().Copy();
    signature_     = rhs.signature().Copy();
    contract_name_ = rhs.contract_name_;
  }

  BasicTransaction &operator=(BasicTransaction const &rhs)
  {
    summary_ = rhs.summary();
    summary_.transaction_hash = rhs.summary().transaction_hash.Copy();

    data_          = rhs.data().Copy();
    signature_     = rhs.signature().Copy();
    contract_name_ = rhs.contract_name_;

    return *this;
  }

  BasicTransaction(BasicTransaction &&rhs)
  {
    summary_ = rhs.summary();
    summary_.transaction_hash = rhs.summary().transaction_hash.Copy();

    data_          = rhs.data().Copy();
    signature_     = rhs.signature().Copy();
    contract_name_ = rhs.contract_name_;
  }

  BasicTransaction &operator=(BasicTransaction&& rhs)
  {
    summary_ = rhs.summary();
    summary_.transaction_hash = rhs.summary().transaction_hash.Copy();

    data_          = rhs.data().Copy();
    signature_     = rhs.signature().Copy();
    contract_name_ = rhs.contract_name_;

    return *this;
  }

protected:

  enum { VERSION = 1 };

  void UpdateDigest()
  {
    LOG_STACK_TRACE_POINT;

    serializers::ByteArrayBuffer buf;
    buf << summary_.groups << signature_ << contract_name_.full_name();
    hasher_type hash;
    hash.Reset();
    hash.Update(buf.data());
    hash.Final();
    summary_.transaction_hash = hash.digest();
  }

  bool operator==(const BasicTransaction &rhs) const
  {
    return digest() == rhs.digest();
  }

  bool operator<(const BasicTransaction &rhs) const
  {

    return digest() < rhs.digest();
  }

  void PushGroup(byte_array::ConstByteArray const &res)
  {
    LOG_STACK_TRACE_POINT;
    summary_.groups.push_back(res);
  }

  void set_contract_name(std::string const &name) {
    contract_name_.Parse(name);
  }

  std::vector<group_type> const &groups() const { return summary_.groups; }

  byte_array::ConstByteArray const &signature() const
  {
    return signature_;
  }

  byte_array::ConstByteArray &signature() { return signature_; }

  byte_array::ConstByteArray &set_signature() { return signature_; }

  ledger::Identifier const &contract_name() const
  {
    return contract_name_;
  }

  ledger::Identifier &contract_name()
  {
    return contract_name_;
  }

  digest_type const &digest() const
  {
    return summary_.transaction_hash;
  }

  byte_array::ConstByteArray data() const { return data_; };

  void set_data(byte_array::ConstByteArray const &data)
  {
    data_ = data;
  }

  TransactionSummary const &summary() const
  {
    return summary_;
  }

  void set_summary(TransactionSummary const &summary)
  {
    summary_ = summary;
  }

private:
  TransactionSummary         summary_;
  byte_array::ConstByteArray data_;
  byte_array::ConstByteArray signature_;
  ledger::Identifier         contract_name_;
};

}
}
#endif
