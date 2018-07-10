#ifndef CHAIN_TRANSACTION_HPP
#define CHAIN_TRANSACTION_HPP

#include "ledger/chain/basic_transaction.hpp"

namespace fetch {
namespace chain {

class Transaction : public BasicTransaction
{
public:
  typedef BasicTransaction super_type;

  Transaction() = default;
  explicit Transaction(super_type &&super) : super_type(super) {
    UpdateDigest();
  }

  digest_type const &digest() const {
    return summary_.transaction_hash;
  }

  bool operator==(const Transaction &rhs) const {
    return digest() == rhs.digest();
  }

  bool operator<(const Transaction &rhs) const {
    return digest() < rhs.digest();
  }


private:

  template <typename T>
  friend inline void Serialize(T &, Transaction const &);
  template <typename T>
  friend inline void Deserialize(T &, Transaction &);
};

} // namespace chain
} // namespace fetch

#endif
