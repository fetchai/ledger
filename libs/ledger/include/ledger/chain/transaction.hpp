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

#include "ledger/chain/mutable_transaction.hpp"

namespace fetch {
namespace chain {

class UnverifiedTransaction : private MutableTransaction
{
public:
  using super_type = MutableTransaction;
  using super_type::VERSION;
  using super_type::hasher_type;
  using super_type::digest_type;
  using super_type::resource_set_type;
  using super_type::signatures_type;
  using super_type::resources;
  using super_type::summary;
  using super_type::data;
  using super_type::signatures;
  using super_type::contract_name;
  using super_type::digest;

  using super_type::operator=;

  bool operator<(UnverifiedTransaction const &other) const { return digest() < other.digest(); }

  MutableTransaction GetMutable() const { return MutableTransaction{*this}; }

protected:
  using super_type::set_summary;
  using super_type::set_data;
  using super_type::set_signatures;
  using super_type::set_contract_name;

  using super_type::UpdateDigest;
  using super_type::Verify;

  template <typename T>
  friend void Serialize(T &serializer, UnverifiedTransaction const &b);

  template <typename T>
  friend void Deserialize(T &serializer, UnverifiedTransaction &b);
};

class VerifiedTransaction : public UnverifiedTransaction
{
public:
  using super_type = UnverifiedTransaction;
  using super_type::hasher_type;
  using super_type::digest_type;
  using super_type::resource_set_type;
  using super_type::signatures_type;

  static VerifiedTransaction Create(fetch::chain::MutableTransaction &&trans)
  {
    return VerifiedTransaction::Create(trans);
  }

  static VerifiedTransaction Create(fetch::chain::MutableTransaction const &trans)
  {
    VerifiedTransaction ret;
    // TODO(private issue #189)
    ret.Finalise(trans);
    return ret;
  }

  static VerifiedTransaction Create(UnverifiedTransaction &&trans)
  {
    return VerifiedTransaction::Create(trans);
  }

  static VerifiedTransaction Create(UnverifiedTransaction const &trans)
  {
    VerifiedTransaction ret;
    // TODO(private issue #189)
    ret.Finalise(trans);
    return ret;
  }

protected:
  using super_type::operator=;

  bool Finalise(fetch::chain::MutableTransaction const &base)
  {
    *this = base;
    UpdateDigest();
    return Verify();
  }

  bool Finalise(UnverifiedTransaction const &base)
  {
    *this = base;
    UpdateDigest();
    return Verify();
  }

  template <typename T>
  friend void Serialize(T &serializer, VerifiedTransaction const &b);

  template <typename T>
  friend void Deserialize(T &serializer, VerifiedTransaction &b);
};

using Transaction = VerifiedTransaction;

}  // namespace chain
}  // namespace fetch
