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
#include "core/serializers/byte_array_buffer.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/serializers/serialisation_argument_wrapper.hpp"
#include "crypto/identity.hpp"
#include "crypto/sha256.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/identifier.hpp"

#include <set>
#include <unordered_map>
#include <vector>
#include <functional>

namespace fetch {
namespace chain {

struct Signature
{
  byte_array::ConstByteArray signature_data;
  byte_array::ConstByteArray type;

  void Clone()
  {
    signature_data = signature_data.Copy();
    type           = type.Copy();
  }
};

using signatures_type = std::unordered_map<crypto::Identity, Signature>;

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
  using contract_id_type  = byte_array::ConstByteArray;
  using resource_set_type = std::set<resource_type>;
  using fee_type          = uint64_t;

  resource_set_type resources;
  digest_type       transaction_hash;
  fee_type          fee{0};

  // TODO(issue 33): Needs to be replaced with some kind of ID
  contract_id_type contract_name;

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

class MutableTransaction;

template<typename MUTABLE_TRANSACTION = MutableTransaction>
class TxDataForSigningC : public std::reference_wrapper<MUTABLE_TRANSACTION> //public TxDataForSigningCBase<MUTABLE_TRANSACTION>
{
  static_assert(std::is_same<MutableTransaction, typename std::remove_const<MUTABLE_TRANSACTION>::type>::value, "Type must be const or non-const `MutableTransaction` class");

public:
  using self_type = TxDataForSigningC;
  using base_type = std::reference_wrapper<MUTABLE_TRANSACTION>;

  //TODO(pbukva) (private issue: Support for switching between different types of signatures)
  using verifier_type = crypto::ECDSAVerifier;
  using signer_type = crypto::ECDSASigner;

  using base_type::base_type;
  //using base_type::operator=;
  using base_type::operator();

  TxDataForSigningC() = default;

  TxDataForSigningC(self_type const &from)
    : base_type{from}
    , stream_{ from.stream_ }
    , tx_data_size_for_signing_{ from.tx_data_size_for_signing_ }
  {
  }

  TxDataForSigningC(self_type &&from)
    : base_type{ std::move(from) }
    , stream_{ std::move(from.stream_) }
    , tx_data_size_for_signing_{ from.tx_data_size_for_signing_ }
  {
  }

  self_type& operator = (self_type const &from)
  {
    static_cast<base_type &>(*this) = from;
    stream_ = from.stream_;
    tx_data_size_for_signing_ = from.tx_data_size_for_signing_;
    return *this;
  }

  self_type& operator = (self_type &&from)
  {
    static_cast<base_type &>(*this) = std::move(from);
    stream_ = std::move(from.stream_);
    tx_data_size_for_signing_ = from.tx_data_size_for_signing_;
    return *this;
  }

  byte_array::ConstByteArray const& DataForSigning(crypto::Identity const& identity)
  {
    return DataForSigningInternal(identity);
  }

  byte_array::ConstByteArray const& DataForSigning()
  {
    return DataForSigningInternal(serializers::LazyEvalArgumentFactory([this](auto&){std::cout << "DataForSigning(): tx_data_size_for_signing_ = " << tx_data_size_for_signing_ << std::endl;}));
  }

  bool Verify(signatures_type::value_type const &sig)
  {
    auto const& identity  = sig.first;
    auto const& signature = sig.second;
   
    verifier_type verifier{identity};
    auto data_to_verify = DataForSigning(identity);
    std::cout << "Verify(sig) : identity       = " << byte_array::ToHex(identity.identifier()) << std::endl;
    std::cout << "Verify(sig) : exp. sig       = " << byte_array::ToHex(signature.signature_data) << std::endl;
    std::cout << "Verify(sig) : data to verify = " << byte_array::ToHex(data_to_verify) << std::endl;
    return verifier.Verify(data_to_verify, signature.signature_data);
  }

  signer_type Sign(byte_array::ConstByteArray const &private_key)
  {
    signer_type signer;
    signer.Load(private_key);
    auto data_to_verify = DataForSigning(signer.identity());
    if (!signer.Sign(data_to_verify))
    {
      throw std::runtime_error("Signing failed");
    }
    std::cout << "Sign(key) : identity       = " << byte_array::ToHex(signer.identity().identifier()) << std::endl;
    std::cout << "Sign(key) : signature      = " << byte_array::ToHex(signer.signature()) << std::endl;
    std::cout << "Sign(key) : data to verify = " << byte_array::ToHex(data_to_verify) << std::endl;
    return signer;
  }

  void Reset()
  {
    stream_.Resize(0);
    //tx_data_size_for_signing_ = 0;
  }

  bool operator == (TxDataForSigningC const &left_tx) const;
  bool operator != (TxDataForSigningC const &left_tx) const;

public:
  using self_ref_type = std::reference_wrapper<TxDataForSigningC>;

  struct qtds : public self_ref_type
  {
    using self_ref_type::self_ref_type;
    //using self_ref_type::operator=;
    using self_ref_type::get;

    template<typename STREAM>
    void operator ()(STREAM& stream) const
    {
      std::cout << "qtds(...)[BEFORE]: tx_data_size_for_signing_ = " << get().tx_data_size_for_signing_ << std::endl;
      get().tx_data_size_for_signing_ = stream.size();
      std::cout << "qtds(...) [AFTER]: tx_data_size_for_signing_ = " << get().tx_data_size_for_signing_ << std::endl;
    }
  };

  struct res : public self_ref_type
  {
    using self_ref_type::self_ref_type;
    //using self_ref_type::operator=;
    using self_ref_type::get;

    template<typename STREAM>
    void operator ()(STREAM& stream) const
    {
      std::cout << "res(...)[BEFORE]: tx_data_size_for_signing_ = " << get().tx_data_size_for_signing_ << std::endl;
      std::cout << "res(...)[BEFORE]: get().stream_.capacity() = " << get().stream_.capacity() << std::endl;
      std::cout << "res(...)[BEFORE]: get().stream_.size() = " << get().stream_.size() << std::endl;
      std::cout << "res(...)[BEFORE]: stream.capacity() = " << stream.capacity() << std::endl;
      std::cout << "res(...)[BEFORE]: stream.size() = " << stream.size() << std::endl;
      stream.Reserve((stream.size() - get().tx_data_size_for_signing_)*10, serializers::eResizeParadigm::relative);
      std::cout << "res(...)[ AFTER]: get().stream_.capacity() = " << get().stream_.capacity() << std::endl;
      std::cout << "res(...)[ AFTER]: get().stream_.size() = " << get().stream_.size() << std::endl;
      std::cout << "res(...)[ AFTER]: stream.capacity() = " << stream.capacity() << std::endl;
      std::cout << "res(...)[ AFTER]: stream.size() = " << stream.size() << std::endl;
    }
  };

  template<typename IDENTITY>
  byte_array::ConstByteArray const& DataForSigningInternal(IDENTITY const& identity)
  {
    std::cout << "DataForSigningInternal(...) [BEFORE]: tx_data_size_for_signing_ = " << tx_data_size_for_signing_ << std::endl;
    if(stream_.size() == 0)
    {
      stream_.Append(*this, qtds_, identity, res_);
    }
    else
    {
      stream_.Resize(tx_data_size_for_signing_, serializers::eResizeParadigm::absolute);
      stream_.Seek(tx_data_size_for_signing_);
      stream_.Append(identity);
    }
    std::cout << "DataForSigningInternal(...) [ AFTER]: tx_data_size_for_signing_ = " << tx_data_size_for_signing_ << std::endl;
    return stream_.data();
  }

  serializers::ByteArrayBuffer stream_;
  std::size_t tx_data_size_for_signing_;
  serializers::LazyEvalArgument<qtds> qtds_{qtds{*this}};
  serializers::LazyEvalArgument<res> res_{res{*this}};
};

template <typename T, typename U>
void Serialize(T &stream, TxDataForSigningC<U> const &tx);

template <typename T, typename U>
void Deserialize(T &stream, TxDataForSigningC<U> &tx);

template<typename MUTABLE_TRANSACTION>
TxDataForSigningC<MUTABLE_TRANSACTION> TxDataForSigningCFactory(MUTABLE_TRANSACTION &tx)
{
  return TxDataForSigningC<MUTABLE_TRANSACTION>(tx);
}

class MutableTransaction
{
public:
  using hasher_type       = crypto::SHA256;
  using digest_type       = TransactionSummary::digest_type;
  using resource_set_type = TransactionSummary::resource_set_type;
  using signatures_type   = signatures_type;
  using tx_data_for_signing_type = TxDataForSigningC<MutableTransaction>;

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
  TransactionSummary::contract_id_type const &contract_name() const
  {
    return summary_.contract_name;
  }
  digest_type const &digest() const
  {
    return summary_.transaction_hash;
  }
  TransactionSummary::fee_type const &fee() const
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

    hasher_type hash;

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

    hash.Update(summary_.fee);
    hash.Update(data_);
    summary_.transaction_hash = hash.Final();
  }

public:

  bool Verify()
  {
    tx_data_for_signing_type txdfs{ *this };
    tx_for_signing_.Reset();
    for( auto const& sig: signatures_)
    {
      //bool const ver_res = txdfs.Verify(sig);
      bool const ver_res = tx_for_signing_.Verify(sig);

      if (!ver_res)
      {
        std::cout << "signature does not match" <<std::endl;
        return false;
      }
    }

    std::cout << "signatures size is zero" <<std::endl;
    return signatures_.size() > 0;
  }

  Signature const& Sign(byte_array::ConstByteArray const &private_key)
  {
    auto signer = tx_for_signing_.Sign(private_key);
    return signatures_[signer.identity()] = Signature{signer.signature(), tx_data_for_signing_type::signer_type::Signature::ecdsa_curve_type::sn};
  }

  void PushResource(byte_array::ConstByteArray const &res)
  {
    LOG_STACK_TRACE_POINT;
    summary_.resources.insert(res);
    tx_for_signing_.Reset();
  }

  void set_summary(TransactionSummary const &summary)
  {
    summary_ = summary;
    tx_for_signing_.Reset();
  }

  void set_data(byte_array::ConstByteArray const &data)
  {
    data_ = data;
    tx_for_signing_.Reset();
  }

  void set_signatures(signatures_type const &sig)
  {
    signatures_ = sig;
  }

  void set_contract_name(TransactionSummary::contract_id_type const &name)
  {
    summary_.contract_name = name;
    tx_for_signing_.Reset();
  }

  void set_fee(uint64_t fee)
  {
    summary_.fee = fee;
    tx_for_signing_.Reset();
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
    tx_for_signing_ = tx_for_signing_;
  }

private:
  TransactionSummary         summary_;
  byte_array::ConstByteArray data_;
  signatures_type            signatures_;
  tx_data_for_signing_type   tx_for_signing_{*this};

  template<typename T>
  friend class TxDataForSigningC;
  template<typename T, typename U>
  friend void Serialize(T &stream, TxDataForSigningC<U> const &tx);
  template<typename T, typename U>
  friend void Deserialize(T &stream, TxDataForSigningC<U> &tx);
};


template <typename T, typename U>
void Serialize(T &stream, TxDataForSigningC<U> const &tx)
{
  MutableTransaction const &tx_ = tx.get();
  stream.Append(tx_.summary_.contract_name, tx_.summary_.fee, tx_.summary_.resources, tx_.data_);
}

template <typename T, typename U>
void Deserialize(T &stream, TxDataForSigningC<U> &tx)
{
  MutableTransaction &tx_ = tx.get();
  stream >> tx_.summary_.contract_name >> tx_.summary_.fee >> tx_.summary_.resources >> tx_.data_;
  tx.Reset();
}

template<typename MUTABLE_TRANSACTION>
inline bool TxDataForSigningC<MUTABLE_TRANSACTION>::operator == (TxDataForSigningC<MUTABLE_TRANSACTION> const &left_tx) const
{
  MutableTransaction &tx_ = left_tx.get();
  MutableTransaction const &left = left_tx.get();
  return tx_.summary_.contract_name == left.summary_.contract_name
      && tx_.summary_.fee == left.summary_.fee
      && tx_.summary_.resources == left.summary_.resources
      && tx_.data_ == left.data_; 
}

template<typename MUTABLE_TRANSACTION>
inline bool TxDataForSigningC<MUTABLE_TRANSACTION>::operator != (TxDataForSigningC<MUTABLE_TRANSACTION> const &left_tx) const
{
  return ! (*this == left_tx);
}

template <typename T>
void Serialize(T &serializer, signatures_type::value_type const &b)
{
  serializer << b.first << b.second;
}

template <typename T>
void Deserialize(T &serializer, signatures_type::value_type &b)
{
  serializer >> b.first >> b.second;
}

}  // namespace chain
}  // namespace fetch
