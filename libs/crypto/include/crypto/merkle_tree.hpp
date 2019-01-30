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

#include "core/byte_array/byte_array.hpp"
#include "crypto/sha256.hpp"
#include "crypto/hash.hpp"
#include "vectorise/platform.hpp"

namespace fetch {
namespace crypto {

class MerkleTree
{
public:
  using HashArray      = byte_array::ConstByteArray;
  using Container = std::map<uint64_t, HashArray>;

  MerkleTree()                                 = default;
  MerkleTree(MerkleTree const &rhs)            = default;
  MerkleTree(MerkleTree &&rhs)                 = default;
  MerkleTree &operator=(MerkleTree const &rhs) = default;
  MerkleTree &operator=(MerkleTree&& rhs)      = default;

  HashArray &operator[](std::size_t const &n)
  {
    return leaf_nodes_[n];
  }

  void CalculateRoot() const
  {
    if(leaf_nodes_.size() == 0)
    {
      root_ = Hash<crypto::SHA256>(HashArray{});
      return;
    }

    std::vector<HashArray> hashes;
    uint64_t          last_index = 0;

    for(auto const &kv_iterator : leaf_nodes_)
    {
      auto &key   = kv_iterator.first;
      auto &value = kv_iterator.second;

      if(kv_iterator == (*leaf_nodes_.cbegin()))
      {
        hashes.push_back(value);
        last_index = key;
        continue;
      }

      // Make sure to fill the vector with absent leaves if necessary
      assert(last_index != 0);

      while(last_index < key - 1)
      {
        hashes.push_back(HashArray{});
        last_index++;
      }

      assert(key == 1 + last_index);

      hashes.push_back(value);
      last_index = key;

      assert(hashes[key] == value);
    }

    // If necessary bump the 'leaves' up to a power of 2
    while(!platform::IsLog2(hashes.size()))
    {
      hashes.push_back(HashArray{});
    }

    // Now, repeatedly condense the vector by calculating the parents of each of the roots
    while(hashes.size() > 1)
    {
      for (std::size_t i = 0; i < hashes.size(); i += 2)
      {
        HashArray concantenated_hash = hashes[i] + hashes[i+1];
        concantenated_hash       = Hash<crypto::SHA256>(concantenated_hash);
        hashes[i]                = concantenated_hash;
      }

      hashes.resize(hashes.size() / 2);
    }

    assert(hashes.size() == 1);
    root_ = hashes[0];
  }

  HashArray const &root() const
  {
    return root_;
  }

  Container const &leaf_nodes() const
  {
    return leaf_nodes_;
  }

private:
  Container     leaf_nodes_;
  mutable HashArray root_;

  template <typename T>
  friend inline void Serialize(T &serializer, MerkleTree const &);

  template <typename T>
  friend inline void Deserialize(T &serializer, MerkleTree &b);
};

template <typename T>
void Serialize(T &serializer, MerkleTree const &tree)
{
  serializer << tree.leaf_nodes_ << tree.root_;
}

template <typename T>
void Deserialize(T &serializer, MerkleTree &tree)
{
  serializer >> tree.leaf_nodes_ >> tree.root_;
}

/*
class Identity
{
public:
  using edcsa_curve_type = crypto::openssl::ECDSACurve<NID_secp256k1>;

  Identity()
  {}

  Identity(Identity const &other) = default;
  Identity &operator=(Identity const &other) = default;
  Identity(Identity &&other)                 = default;
  Identity &operator=(Identity &&other) = default;

  // Fully relying on caller that it will behave = will NOT modify value passed
  // (Const)ByteArray(s)
  Identity(byte_array::ConstByteArray identity_parameters, byte_array::ConstByteArray identifier)
    : identity_parameters_{std::move(identity_parameters)}
    , identifier_{std::move(identifier)}
  {}

  Identity(byte_array::ConstByteArray identifier)
    : identifier_{std::move(identifier)}
  {}

  byte_array::ConstByteArray const &parameters() const
  {
    return identity_parameters_;
  }

  byte_array::ConstByteArray const &identifier() const
  {
    return identifier_;
  }

  void SetIdentifier(byte_array::ConstByteArray const &ident)
  {
    identifier_ = ident;
  }

  void SetParameters(byte_array::ConstByteArray const &params)
  {
    identity_parameters_ = params;
  }

  operator bool() const
  {
    return identity_parameters_ == edcsa_curve_type::sn &&
           identifier_.size() == edcsa_curve_type::publicKeySize;
  }

  static Identity CreateInvalid()
  {
    Identity id;
    return id;
  }

  bool operator==(Identity const &right) const
  {
    return identity_parameters_ == right.identity_parameters_ && identifier_ == right.identifier_;
  }

  bool operator<(Identity const &right) const
  {
    if (identifier_ < right.identifier_)
    {
      return true;
    }
    else if (identifier_ == right.identifier_)
    {
      return identity_parameters_ < right.identity_parameters_;
    }

    return false;
  }

  void Clone()
  {
    identifier_          = identifier_.Copy();
    identity_parameters_ = identity_parameters_.Copy();
  }

private:
  byte_array::ConstByteArray identity_parameters_{edcsa_curve_type::sn};
  byte_array::ConstByteArray identifier_;
};

static inline Identity InvalidIdentity()
{
  return Identity::CreateInvalid();
}

template <typename T>
T &Serialize(T &serializer, Identity const &data)
{
  serializer << data.identifier();
  serializer << data.parameters();

  return serializer;
}

template <typename T>
T &Deserialize(T &serializer, Identity &data)
{
  byte_array::ByteArray params, id;
  serializer >> id;
  serializer >> params;

  data.SetParameters(params);
  data.SetIdentifier(id);
  if (!data)
  {
    data = InvalidIdentity();
  }

  return serializer;
}
 */
}  // namespace crypto
}  // namespace fetch

namespace std {

}  // namespace std
