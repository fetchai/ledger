#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP
#include <crypto/sha256.hpp>

#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/referenced_byte_array.hpp"
#include "core/serializers/typed_byte_array_buffer.hpp"

namespace fetch {
typedef uint16_t group_type;

namespace chain {

struct TransactionSummary {
  typedef byte_array::ConstByteArray digest_type;
  std::vector<group_type> groups;
  digest_type transaction_hash;

  double fee = 0.0;
  uint64_t short_id;
};

template <typename T>
void Serialize(T &serializer, TransactionSummary const &b) {
  serializer << b.groups << b.fee << b.transaction_hash;
}

template <typename T>
void Deserialize(T &serializer, TransactionSummary &b) {
  serializer >> b.groups >> b.fee >> b.transaction_hash;
}

class Transaction {
 public:
  typedef crypto::SHA256 hasher_type;
  typedef TransactionSummary::digest_type digest_type;
  typedef byte_array::ConstByteArray
      arguments_type;  // TODO: json doc with native serialization

  enum { VERSION = 1 };

  void UpdateDigest() {
    LOG_STACK_TRACE_POINT;

    serializers::ByteArrayBuffer buf;
    buf << summary_.groups << signatures_ << contract_name_ << arguments_;
    hasher_type hash;
    hash.Reset();
    hash.Update(buf.data());
    hash.Final();
    summary_.transaction_hash = hash.digest();
  }

  bool operator==(const Transaction &rhs) const {
    return digest() == rhs.digest();
  }

  bool operator<(const Transaction &rhs) const {
    return digest() < rhs.digest();
  }

  void PushGroup(byte_array::ConstByteArray const &res) {
    LOG_STACK_TRACE_POINT;
    union {
      uint8_t bytes[2];
      uint16_t value;
    } d;
    d.value = 0;

    switch (res.size()) {
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
    //    std::cout << byte_array::ToHex( res) << " >> " << d.value <<
    //    std::endl;

    //    assert(d.value < 10);

    PushGroup(d.value);
  }

  void PushGroup(group_type const &res) {
    LOG_STACK_TRACE_POINT;
    bool add = true;
    for (auto &g : summary_.groups) {
      if (g == res) {
        add = false;
        break;
      }
    }

    if (add) {
      summary_.groups.push_back(res);
    }
  }

  bool UsesGroup(group_type g, group_type m) const {
    --m;
    g &= m;

    bool ret = false;
    for (auto const &gg : summary_.groups) {
      ret |= (g == (gg & m));
    }

    return ret;
  }

  void PushSignature(byte_array::ConstByteArray const &sig) {
    LOG_STACK_TRACE_POINT;
    signatures_.push_back(sig);
  }

  void set_contract_name(byte_array::ConstByteArray const &name) {
    contract_name_ = name;
  }

  void set_arguments(byte_array::ConstByteArray const &args) {
    arguments_ = args;
  }

  std::vector<group_type> const &groups() const { return summary_.groups; }

  std::vector<byte_array::ConstByteArray> const &signatures() const {
    return signatures_;
  }

  std::vector<byte_array::ConstByteArray> &signatures() { return signatures_; }

  byte_array::ConstByteArray const &contract_name() const {
    return contract_name_;
  }

  arguments_type const &arguments() const { return arguments_; }
  digest_type const &digest() const {

    return summary_.transaction_hash;
  }

  uint32_t signature_count() const { return signature_count_; }

  byte_array::ConstByteArray data() const { return data_; };

  TransactionSummary const &summary() const {
    return summary_;
  }

 private:
  TransactionSummary summary_;

  uint32_t signature_count_;
  byte_array::ConstByteArray data_;
  // TODO: Add resources
  std::vector<byte_array::ConstByteArray> signatures_;
  byte_array::ConstByteArray contract_name_;

  arguments_type arguments_;

  template <typename T>
  friend inline void Serialize(T &, Transaction const &);
  template <typename T>
  friend inline void Deserialize(T &, Transaction &);
};

template <typename T>
inline void Serialize(T &serializer, Transaction const &b) {
  serializer << uint16_t(b.VERSION);

  serializer << b.summary_;

  serializer << uint32_t(b.signatures().size());
  for (auto &sig : b.signatures()) {
    serializer << sig;
  }

  serializer << b.contract_name();
  serializer << b.arguments();
}

template <typename T>
inline void Deserialize(T &serializer, Transaction &b) {
  uint16_t version;
  serializer >> version;

  serializer >> b.summary_;

  uint32_t size;

  serializer >> size;
  for (std::size_t i = 0; i < size; ++i) {
    byte_array::ByteArray sig;
    serializer >> sig;
    b.PushSignature(sig);
  }

  byte_array::ByteArray contract_name;
  typename Transaction::arguments_type arguments;
  serializer >> contract_name >> arguments;

  b.set_contract_name(contract_name);
  b.set_arguments(arguments);
}
}
}
#endif
