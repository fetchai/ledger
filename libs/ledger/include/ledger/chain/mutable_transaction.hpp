#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "core/serializers/serialisation_verbatim_wrapper.hpp"
#include "core/serializers/stl_types.hpp"
#include "crypto/ecdsa.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "ledger/identifier.hpp"

#include <functional>
#include <set>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace ledger {

struct Signature
{
  byte_array::ConstByteArray signature_data;
  byte_array::ConstByteArray type;
};

using Signatories = std::unordered_map<crypto::Identity, Signature>;
using Signatory   = Signatories::value_type;

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
  using Resource       = byte_array::ConstByteArray;
  using TxDigest       = byte_array::ConstByteArray;
  using ContractName   = byte_array::ConstByteArray;
  using ResourceSet    = std::set<Resource>;
  using RawResourceSet = std::set<Resource>;
  using Fee            = uint64_t;

  ResourceSet    resources;
  RawResourceSet raw_resources;  // Raw hashes (not wrapped by scope)

  TxDigest transaction_hash;
  Fee      fee{0};

  // TODO(issue 33): Needs to be replaced with some kind of ID
  ContractName contract_name;

  bool operator==(TransactionSummary const &rhs) const
  {
    return transaction_hash == rhs.transaction_hash;
  }

  bool operator<(TransactionSummary const &rhs) const
  {
    return transaction_hash < rhs.transaction_hash;
  }

  bool operator>(TransactionSummary const &rhs) const
  {
    return !(*this < rhs);
  }

  bool IsWellFormed() const
  {
    if (resources.size() > 0 && transaction_hash.size() > 0 && contract_name.size() > 0)
    {
      for (auto const &hash : raw_resources)
      {
        if (hash.size() == 0)
        {
          FETCH_LOG_INFO("TransactionSummary",
                         "Found invalid TX: smart contact hash ref size: ", hash.size());
          return false;
        }
      }

      return true;
    }
    else
    {
      return false;
    }
  }
};

template <typename T>
void Serialize(T &serializer, TransactionSummary const &b)
{
  serializer << b.resources << b.raw_resources << b.fee << b.transaction_hash << b.contract_name;
}

template <typename T>
void Deserialize(T &serializer, TransactionSummary &b)
{
  serializer >> b.resources >> b.raw_resources >> b.fee >> b.transaction_hash >> b.contract_name;
}

class MutableTransaction;

// TODO(HUT):  de-template this if it's only using mutable TX
template <typename MUTABLE_TRANSACTION = MutableTransaction>
class TxSigningAdapter
{
  static_assert(std::is_same<MutableTransaction,
                             typename std::remove_const<MUTABLE_TRANSACTION>::type>::value,
                "Type must be const or non-const `MutableTransaction` class");

public:
  using self_type        = TxSigningAdapter;
  using transaction_type = MUTABLE_TRANSACTION;

  // TODO(issue #260): Runtime switching between different types cryptographic schemes
  using signature_type   = typename crypto::ECDSASigner::Signature;
  using private_key_type = crypto::openssl::ECDSAPrivateKey<>;
  using public_key_type  = private_key_type::public_key_type;
  using hasher_type      = crypto::ECDSASigner::Signature::hasher_type;

  TxSigningAdapter() = delete;

  TxSigningAdapter(self_type const &from) = default;
  TxSigningAdapter(self_type &&from)      = default;
  self_type &operator=(self_type const &from) = default;
  self_type &operator=(self_type &&from) = default;

  TxSigningAdapter(transaction_type &tx)
    : tx_{&tx}
  {}

  TxSigningAdapter &operator=(transaction_type &tx)
  {
    tx_ = &tx;
    Reset();
    Update();
    return *this;
  }

  operator transaction_type &() const
  {
    if (!tx_)
    {
      throw std::runtime_error("Pointer to wrapped underlying transaction is nullptr.");
    }

    return *tx_;
  }

  byte_array::ConstByteArray const &DataForSigning() const
  {
    Update();
    return stream_->data();
  }

  bool Verify(Signatory const &sig) const
  {
    auto const &    identity  = sig.first;
    auto const &    signature = sig.second;
    public_key_type pub_key{identity.identifier()};
    auto            hash = HashOfTxDataForSigning(identity);
    return signature_type{signature.signature_data}.VerifyHash(pub_key, hash);
  }

  Signatory Sign(byte_array::ConstByteArray const &private_key) const
  {
    return Sign(private_key_type{private_key});
  }

  Signatory Sign(private_key_type const &private_key) const
  {
    crypto::Identity identity{signature_type::ecdsa_curve_type::sn,
                              private_key.publicKey().keyAsBin()};
    auto             hash = HashOfTxDataForSigning(identity);
    auto             sig  = signature_type::SignHash(private_key, hash);
    return Signatory{std::move(identity),
                     Signature{sig.signature(), signature_type::ecdsa_curve_type::sn}};
  }

  void Reset()
  {
    stream_->Resize(0);
    tx_data_hash_->Reset();
  }

  void Update() const;

  byte_array::ConstByteArray HashOfTxDataForSigning(crypto::Identity const &identity) const
  {
    Update();

    // Copy hash context by *value*
    hasher_type tx_data_hash_copy = *tx_data_hash_;
    identity_stream_->Resize(0, ResizeParadigm::ABSOLUTE);
    *identity_stream_ << identity;

    // Adding serialized identity to the hash for signing
    if (!tx_data_hash_copy.Update(identity_stream_->data()))
    {
      throw std::runtime_error("Failure while updating hash for signing");
    }

#if !defined(NDEBUG) && defined(TX_SIGNING_DBG_OUTPUT)
    auto const prod_digest = tx_data_hash_copy.Final();
    std::cerr << "prod. digest           [hex]: " << byte_array::ToHex(prod_digest) << std::endl;
    std::cerr << "prod. tx data          [hex]: " << byte_array::ToHex(stream_->data())
              << std::endl;
    std::cerr << "prod. identity         [hex]: " << byte_array::ToHex(identity_stream_->data())
              << std::endl;

    hasher_type                  hasher;
    serializers::ByteArrayBuffer stream;
    stream << serializers::Verbatim(stream_->data()) << identity;
    hasher.Update(stream.data());

    std::cerr << "real digest            [hex]: " << byte_array::ToHex(hasher.Final()) << std::endl;
    std::cerr << "real full data to sig. [hex]: " << byte_array::ToHex(stream.data()) << std::endl;

    return prod_digest;
#else
    return tx_data_hash_copy.Final();
#endif
  }

  bool operator==(TxSigningAdapter const &left_tx) const;
  bool operator!=(TxSigningAdapter const &left_tx) const;

private:
  transaction_type *tx_ = nullptr;
  // Data members as shared pointers to avoid issue with const-ness issues when modifying content.
  std::shared_ptr<serializers::ByteArrayBuffer> stream_{
      std::make_shared<serializers::ByteArrayBuffer>()};
  std::shared_ptr<hasher_type>                  tx_data_hash_{std::make_shared<hasher_type>()};
  std::shared_ptr<serializers::ByteArrayBuffer> identity_stream_{
      std::make_shared<serializers::ByteArrayBuffer>()};

  template <typename T, typename U>
  friend void Serialize(T &stream, TxSigningAdapter<U> const &tx);
  template <typename T, typename U>
  friend void Deserialize(T &stream, TxSigningAdapter<U> &tx);
};

template <typename T, typename U>
void Serialize(T &stream, TxSigningAdapter<U> const &tx);

template <typename T, typename U>
void Deserialize(T &stream, TxSigningAdapter<U> &tx);

template <typename MUTABLE_TX>
TxSigningAdapter<MUTABLE_TX> TxSigningAdapterFactory(MUTABLE_TX &tx)
{
  return TxSigningAdapter<MUTABLE_TX>{tx};
}

class MutableTransaction
{
public:
  using Hasher         = crypto::SHA256;
  using TxDigest       = TransactionSummary::TxDigest;
  using ResourceSet    = TransactionSummary::ResourceSet;
  using SigningAdapter = TxSigningAdapter<MutableTransaction>;

  ResourceSet const &resources() const
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

  Signatories const &signatures() const
  {
    return signatures_;
  }

  TransactionSummary::ContractName const &contract_name() const
  {
    return summary_.contract_name;
  }

  TransactionSummary::RawResourceSet const &raw_resources() const
  {
    return summary_.raw_resources;
  }

  TxDigest const &digest() const
  {
    return summary_.transaction_hash;
  }

  TransactionSummary::Fee const &fee() const
  {
    return summary_.fee;
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

    Hasher hash;

    std::vector<std::pair<crypto::Identity, Signature> /*typename signatures_type::value_type*/>
        signatures;
    std::copy(signatures_.begin(), signatures_.end(), std::back_inserter(signatures));
    //* Signatures are sorted based on associated Identity value
    std::sort(signatures.begin(), signatures.end(),
              [](Signatory const &a, Signatory const &b) -> bool { return a.first < b.first; });

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
    resources.reserve(summary().resources.size());

    std::copy(summary().resources.begin(), summary().resources.end(),
              std::back_inserter(resources));
    std::sort(resources.begin(), resources.end());
    for (auto const &e : resources)
    {
      hash.Update(e);
    }

    std::vector<byte_array::ConstByteArray> raw_resources;
    raw_resources.reserve(summary().raw_resources.size());

    std::copy(summary().raw_resources.begin(), summary().raw_resources.end(),
              std::back_inserter(raw_resources));
    std::sort(raw_resources.begin(), raw_resources.end());
    for (auto const &e : raw_resources)
    {
      hash.Update(e);
    }

    hash.Update(summary_.fee);
    hash.Update(data_);
    summary_.transaction_hash = hash.Final();
  }

  bool Verify() const
  {
    auto tx_sign_adapter = TxSigningAdapterFactory(*this);
    return Verify(tx_sign_adapter);
  }

  template <typename MUTABLE_TX>
  bool Verify(TxSigningAdapter<MUTABLE_TX> &tx_sign_adapter) const
  {
    for (auto const &sig : signatures_)
    {
      if (sig.first.identifier().empty())
      {
        FETCH_LOG_WARN("TxVerify", "Failed to validate the signature because the identity is not there");
        throw std::runtime_error("Empty identity error");
      }

      bool const ver_res = tx_sign_adapter.Verify(sig);

      if (!ver_res)
      {
        return false;
      }
    }

    return signatures_.size() > 0;
  }

  Signature const &Sign(byte_array::ConstByteArray const &private_key,
                        SigningAdapter &                  tx_sign_adapter)
  {
    return SignInternal(private_key, tx_sign_adapter);
  }

  Signature const &Sign(SigningAdapter::private_key_type const &private_key,
                        SigningAdapter &                        tx_sign_adapter)
  {
    return SignInternal(private_key, tx_sign_adapter);
  }

  Signature const &Sign(byte_array::ConstByteArray const &private_key)
  {
    SigningAdapter tx_sign_adapter{*this};
    return SignInternal(private_key, tx_sign_adapter);
  }

  Signature const &Sign(SigningAdapter::private_key_type const &private_key)
  {
    SigningAdapter tx_sign_adapter{*this};
    return SignInternal(private_key, tx_sign_adapter);
  }

  void PushResource(byte_array::ConstByteArray const &res)
  {
    LOG_STACK_TRACE_POINT;
    summary_.resources.insert(res);
  }

  void PushContractHash(byte_array::ConstByteArray const &res)
  {
    summary_.raw_resources.insert(res);
  }

  void set_summary(TransactionSummary const &summary)
  {
    summary_ = summary;
  }

  void set_data(byte_array::ConstByteArray const &data)
  {
    data_ = data;
  }

  void set_signatures(Signatories const &sig)
  {
    signatures_ = sig;
  }

  void set_contract_name(TransactionSummary::ContractName const &name)
  {
    summary_.contract_name = name;
  }

  void set_contract_hash(TransactionSummary::RawResourceSet const &hashes)
  {
    summary_.raw_resources = hashes;
  }

  void set_fee(uint64_t fee)
  {
    summary_.fee = fee;
  }

  void set_resources(TransactionSummary::ResourceSet resources)
  {
    summary_.resources = std::move(resources);
  }

  bool operator==(MutableTransaction const &rhs) const
  {
    return summary_.transaction_hash == rhs.summary().transaction_hash;
  }

  bool operator>(MutableTransaction const &rhs) const
  {
    return summary_.transaction_hash > rhs.summary().transaction_hash;
  }

private:
  TransactionSummary         summary_;
  byte_array::ConstByteArray data_;
  Signatories                signatures_;

  template <typename PRIVATE_KEY_TYPE>
  Signature const &SignInternal(PRIVATE_KEY_TYPE const &private_key,
                                SigningAdapter &        tx_sign_adapter)
  {
    auto const result = signatures_.emplace(tx_sign_adapter.Sign(private_key));
    if (!result.second)
    {
      throw std::runtime_error("Signature for given private key already already exists.");
    }
    return result.first->second;
  }

  template <typename MUTABLE_TX>
  friend class TxSigningAdapter;
  template <typename T, typename MUTABLE_TX>
  friend void Serialize(T &stream, TxSigningAdapter<MUTABLE_TX> const &tx);
  template <typename T, typename MUTABLE_TX>
  friend void Deserialize(T &stream, TxSigningAdapter<MUTABLE_TX> &tx);
};

template <typename MUTABLE_TX>
void TxSigningAdapter<MUTABLE_TX>::Update() const
{
  if (stream_->size() == 0)
  {
    serializers::ByteArrayBuffer &stream = *stream_.get();
    stream.Append(tx_->contract_name(), tx_->fee(), tx_->resources(), tx_->raw_resources(),
                  tx_->data());
    // tx_data_hash_.Reset();
    tx_data_hash_->Update(stream.data());
  }
}

template <typename T, typename MUTABLE_TX>
void Serialize(T &stream, TxSigningAdapter<MUTABLE_TX> const &tx)
{
  stream.Append(serializers::Verbatim{tx.DataForSigning()},
                static_cast<MUTABLE_TX &>(tx).signatures());
}

template <typename T, typename MUTABLE_TX>
void Deserialize(T &stream, TxSigningAdapter<MUTABLE_TX> &tx)
{
  MutableTransaction &tx_ = tx;
  stream >> tx_.summary_.contract_name;
  stream >> tx_.summary_.fee;
  stream >> tx_.summary_.resources;
  stream >> tx_.summary_.raw_resources;
  stream >> tx_.data_;
  stream >> tx_.signatures_;

  tx.Reset();
}

template <typename MUTABLE_TX>
bool TxSigningAdapter<MUTABLE_TX>::operator==(TxSigningAdapter<MUTABLE_TX> const &left_tx) const
{
  MutableTransaction const &left = left_tx;
  return tx_->contract_name() == left.contract_name() && tx_->fee() == left.fee() &&
         tx_->resources() == left.resources() && tx_->raw_resources() == left.raw_resources() &&
         tx_->data() == left.data();
}

template <typename MUTABLE_TX>
bool TxSigningAdapter<MUTABLE_TX>::operator!=(TxSigningAdapter<MUTABLE_TX> const &left_tx) const

{
  return !(*this == left_tx);
}

template <typename T>
void Serialize(T &serializer, Signatory const &b)
{
  serializer << b.first << b.second;
}

template <typename T>
void Deserialize(T &serializer, Signatory &b)
{
  serializer >> b.first >> b.second;
}

}  // namespace ledger
}  // namespace fetch
