#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/const_byte_array.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "ledger/identifier.hpp"

#include <set>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace chain {

class Signature
{
public:
  byte_array::ConstByteArray signature_data;
  byte_array::ConstByteArray type;

  bool operator==(Signature const &right) const
  {
    return signature_data == right.signature_data && type == right.type;
  }

  void Clone()
  {
    signature_data = signature_data.Copy();
    type           = type.Copy();
  }
};

template <typename T>
void Serialize(T &serializer, Signature const &b)
{
  serializer << b.signature_data << b.type;
}

template <typename T>
void Deserialize(T &serializer, Signature &b)
{
  serializer >> b.signature_data >> b.type;
}

struct TransactionSummary
{
  using resource_type     = byte_array::ConstByteArray;
  using digest_type       = byte_array::ConstByteArray;
  using contracti_id_type = std::string;
  using resource_set_type = std::set<resource_type>;

  resource_set_type resources;
  digest_type       transaction_hash;
  uint64_t          fee{0};

  // TODO(issue 33): Needs to be replaced with some kind of ID
  contracti_id_type contract_name;

  void Clone()
  {
    resource_set_type res_clone;
    for (auto const &res : resources)
    {
      res_clone.insert(res.Copy());
    }
    resources = std::move(res_clone);

    transaction_hash = transaction_hash.Copy();
  }
};

template <typename T>
void Serialize(T &serializer, TransactionSummary const &b)
{
  serializer << b.resources << b.fee << b.transaction_hash << b.contract_name;
}

template <typename T>
void Deserialize(T &serializer, TransactionSummary &b)
{
  serializer >> b.resources >> b.fee >> b.transaction_hash >> b.contract_name;
}

class MutableTransaction
{
public:
  using hasher_type       = crypto::SHA256;
  using digest_type       = TransactionSummary::digest_type;
  using resource_set_type = TransactionSummary::resource_set_type;
  using signatures_type   = std::unordered_map<crypto::Identity, Signature>;

  resource_set_type const &resources() const
  {
    return summary_.resources;
  }
  TransactionSummary const &summary() const
  {
    return summary_;
  }
  byte_array::ConstByteArray const &data() const
  {
    return data_;
  }
  signatures_type const &signatures() const
  {
    return signatures_;
  }
  TransactionSummary::contracti_id_type const &contract_name() const
  {
    return summary_.contract_name;
  }
  digest_type const &digest() const
  {
    return summary_.transaction_hash;
  }

  enum
  {
    VERSION = 1
  };

  void UpdateDigest()
  {
    LOG_STACK_TRACE_POINT;

    // This is annoying but we should maintain that the fields are
    // kept in order

    hasher_type hash;
    hash.Reset();

    std::vector<std::pair<crypto::Identity, Signature> /*typename signatures_type::value_type*/>
        signatures;
    std::copy(signatures_.begin(), signatures_.end(), std::back_inserter(signatures));
    //* Signatures are sorted based on associated Identity value
    std::sort(signatures.begin(), signatures.end(),
              [](signatures_type::value_type const &a,
                 signatures_type::value_type const &b) -> bool { return a.first < b.first; });

    for (auto const &e : signatures)
    {
      crypto::Identity const &identity = e.first;
      hash.Update(identity.identifier());
      hash.Update(identity.parameters());

      Signature const &signature = e.second;
      hash.Update(signature.signature_data);
      hash.Update(signature.type);
    }

    std::vector<byte_array::ConstByteArray> resources;
    std::copy(summary().resources.begin(), summary().resources.end(),
              std::back_inserter(resources));
    std::sort(resources.begin(), resources.end());
    for (auto const &e : resources)
    {
      hash.Update(e);
    }

    hash.Update(reinterpret_cast<uint8_t const *>(&summary_.fee), sizeof(summary_.fee));
    hash.Update(reinterpret_cast<uint8_t const *>(summary_.contract_name.data()),
                summary_.contract_name.size());
    hash.Update(data_);
    summary_.transaction_hash = hash.Final();
  }

  bool Verify()
  {
    // TODO(issue 24): Needs implementing
    return true;
  }

  void PushResource(byte_array::ConstByteArray const &res)
  {
    LOG_STACK_TRACE_POINT;
    summary_.resources.insert(res);
  }

  void set_summary(TransactionSummary const &summary)
  {
    summary_ = summary;
  }

  void set_data(byte_array::ConstByteArray const &data)
  {
    data_ = data;
  }

  void set_signatures(signatures_type const &sig)
  {
    signatures_ = sig;
  }

  void set_contract_name(TransactionSummary::contracti_id_type const &name)
  {
    summary_.contract_name = name;
  }

  void set_fee(uint64_t fee)
  {
    summary_.fee = fee;
  }

protected:
  void Clone()
  {
    summary_.Clone();
    data_ = data_.Copy();
    signatures_type sigs_clone;
    for (auto &sig : signatures_)
    {
      crypto::Identity identity{sig.first};
      identity.Clone();
      sig.second.Clone();
      sigs_clone[identity] = std::move(sig.second);
    }
    signatures_ = std::move(sigs_clone);
  }

private:
  TransactionSummary         summary_;
  byte_array::ConstByteArray data_;
  signatures_type            signatures_;
};

template <typename T>
void Serialize(T &serializer, MutableTransaction::signatures_type::value_type const &b)
{
  serializer << b.first << b.second;
}

template <typename T>
void Deserialize(T &serializer, MutableTransaction::signatures_type::value_type &b)
{
  serializer >> b.first >> b.second;
}

}  // namespace chain
}  // namespace fetch
