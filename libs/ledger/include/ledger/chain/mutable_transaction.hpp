#pragma once

#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/sha256.hpp"
#include "ledger/identifier.hpp"

#include <set>
#include <vector>

namespace fetch {
namespace chain {

struct TransactionSummary
{
  using resource_type     = byte_array::ConstByteArray;
  using digest_type       = byte_array::ConstByteArray;
  using resource_set_type = std::set<resource_type>;

  resource_set_type resources;
  digest_type       transaction_hash;
  uint64_t          fee{0};

  // TODO: (EJF) Needs to be replaced with some kind of ID
  std::string contract_name_;
};

template <typename T>
void Serialize(T &serializer, TransactionSummary const &b)
{
  serializer << b.resources << b.fee << b.transaction_hash << b.contract_name_;
}

template <typename T>
void Deserialize(T &serializer, TransactionSummary &b)
{
  serializer >> b.resources >> b.fee >> b.transaction_hash >> b.contract_name_;
}

class MutableTransaction
{
public:
  typedef crypto::SHA256                        hasher_type;
  typedef TransactionSummary::digest_type       digest_type;
  typedef TransactionSummary::resource_set_type resource_set_type;

  resource_set_type const &resources() const { return summary_.resources; }

  TransactionSummary const &summary() const { return summary_; }

  byte_array::ConstByteArray const &data() const { return data_; }

  byte_array::ConstByteArray const &signature() const { return signature_; }

  std::string const &contract_name() const { return summary_.contract_name_; }

  digest_type const &digest() const { return summary_.transaction_hash; }

  void Copy(MutableTransaction const &rhs)
  {
    copy_on_write_ = true;

    summary_   = rhs.summary();
    data_      = rhs.data();
    signature_ = rhs.signature();
  }

  MutableTransaction()                              = default;
  MutableTransaction(MutableTransaction const &rhs) = delete;
  MutableTransaction &operator=(MutableTransaction const &rhs) = delete;
  MutableTransaction(MutableTransaction &&rhs)                 = default;
  MutableTransaction &operator=(MutableTransaction &&rhs) = default;

  enum
  {
    VERSION = 1
  };

  void UpdateDigest()
  {
    LOG_STACK_TRACE_POINT;

    // TODO: (EJF) This is annoying but we should maintain that the fields are
    // kept in order
    std::vector<byte_array::ConstByteArray> resources;
    std::copy(summary().resources.begin(), summary().resources.end(),
              std::back_inserter(resources));
    std::sort(resources.begin(), resources.end());

    serializers::ByteArrayBuffer buf;
    for (auto const &e : resources)
    {
      buf << e;
    }

    buf << summary_.fee << summary_.contract_name_ << data_ << signature_;

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

  void PushResource(byte_array::ConstByteArray const &res)
  {
    LOG_STACK_TRACE_POINT;
    if (copy_on_write_) Clone();
    summary_.resources.insert(res);
  }

  void set_summary(TransactionSummary const &summary)
  {
    if (copy_on_write_) Clone();
    summary_ = summary;
  }

  void set_data(byte_array::ConstByteArray const &data)
  {
    if (copy_on_write_) Clone();
    data_ = data;
  }

  void set_signature(byte_array::ConstByteArray const &sig)
  {
    if (copy_on_write_) Clone();
    signature_ = sig;
  }

  void set_contract_name(std::string const &name) { summary_.contract_name_ = name; }

  void set_fee(uint64_t fee) { summary_.fee = fee; }

protected:
  mutable bool copy_on_write_ = false;

  void Clone()
  {
    copy_on_write_            = false;
    summary_.transaction_hash = this->summary().transaction_hash.Copy();
    data_                     = this->data().Copy();
    signature_                = this->signature().Copy();
  }

private:
  TransactionSummary         summary_;
  byte_array::ConstByteArray data_;
  byte_array::ConstByteArray signature_;
};

}  // namespace chain
}  // namespace fetch
