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
#include "math/bignumber.hpp"
#include "core/serializers/group_definitions.hpp"

#include <cstddef>
#include <utility>

namespace fetch {
namespace ledger {
namespace consensus {

class ProofOfWork : public math::BigUnsigned
{
public:
  using super_type  = math::BigUnsigned;
  using header_type = byte_array::ConstByteArray;

  // Construction / Destruction
  ProofOfWork() = default;
  explicit ProofOfWork(header_type header);
  ~ProofOfWork() = default;

  bool operator()();

  void SetTarget(std::size_t zeros);
  void SetTarget(math::BigUnsigned &&target);
  void SetHeader(byte_array::ByteArray header);

  header_type const &      header() const;
  math::BigUnsigned const &digest() const;
  math::BigUnsigned const &target() const;

private:
  math::BigUnsigned          digest_;
  math::BigUnsigned          target_;
  byte_array::ConstByteArray header_;
};

inline byte_array::ConstByteArray const &ProofOfWork::header() const
{
  return header_;
}

inline math::BigUnsigned const &ProofOfWork::digest() const
{
  return digest_;
}

inline math::BigUnsigned const &ProofOfWork::target() const
{
  return target_;
}

} // namespace consensus
} // namespace ledger

namespace serializers
{

template< typename D >
struct ForwardSerializer< math::BigUnsigned, D >
{
public:
  using Type = math::BigUnsigned; 
  using DriverType = D;

  template< typename Serializer >
  static inline void Serialize(Serializer &buffer, Type const &object)
  {
    byte_array::ConstByteArray const & ba = static_cast< byte_array::ConstByteArray const & >(object);
    buffer << ba;
  }

  template< typename Serializer >
  static inline void Deserialize(Serializer &buffer, Type &object)
  {
    byte_array::ConstByteArray & ba = static_cast< byte_array::ConstByteArray & >(object);
    buffer >> ba;
  }
};

template< typename D >
struct MapSerializer< ledger::consensus::ProofOfWork, D > // TODO: Consider using forward to bytearray
{
public:
  using Type = ledger::consensus::ProofOfWork;
  using DriverType = D;

  static uint8_t const HEADER = 1;
  static uint8_t const TARGET = 2;

  template< typename Constructor >
  static void Serialize(Constructor & map_constructor, Type const & p)
  {
    auto map = map_constructor(2);
    map.Append(HEADER, p.header());
    map.Append(TARGET, p.target());
  }

  template< typename MapDeserializer >
  static void Deserialize(MapDeserializer & map, Type & p)
  {
    byte_array::ConstByteArray header;
    math::BigUnsigned          target;
  
    map.ExpectKeyGetValue(HEADER, header);
    map.ExpectKeyGetValue(TARGET, target);

    p.SetHeader(header);
    p.SetTarget(std::move(target));    
  }  
};


}  // namespace serializers
}  // namespace fetch
