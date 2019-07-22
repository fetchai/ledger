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
#include "core/byte_array/const_byte_array.hpp"
#include "core/serializers/group_definitions.hpp"
#include "vectorise/uint/uint.hpp"

#include <cstddef>
#include <utility>

namespace fetch {
namespace ledger {
namespace consensus {

class ProofOfWork : public vectorise::UInt<256>
{
public:
  using UInt256     = vectorise::UInt<256>;
  using header_type = byte_array::ConstByteArray;

  // Construction / Destruction
  ProofOfWork() = default;
  explicit ProofOfWork(header_type header);
  ~ProofOfWork() = default;

  bool operator()();

  void SetTarget(std::size_t zeros);
  void SetTarget(UInt256 &&target);
  void SetHeader(byte_array::ByteArray header);

  header_type const &header() const;
  UInt256 const &    digest() const;
  UInt256 const &    target() const;

private:
  UInt256                    digest_;
  UInt256                    target_;
  byte_array::ConstByteArray header_;
};

inline byte_array::ConstByteArray const &ProofOfWork::header() const
{
  return header_;
}

inline vectorise::UInt<256> const &ProofOfWork::digest() const
{
  return digest_;
}

inline vectorise::UInt<256> const &ProofOfWork::target() const
{
  return target_;
}

}  // namespace consensus
}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::consensus::ProofOfWork,
                     D>  // TODO: Consider using forward to bytearray
{
public:
  using Type       = ledger::consensus::ProofOfWork;
  using DriverType = D;

  static uint8_t const HEADER = 1;
  static uint8_t const TARGET = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &p)
  {
    auto map = map_constructor(2);
    map.Append(HEADER, p.header());
    map.Append(TARGET, p.target());
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &p)
  {
    byte_array::ConstByteArray header;
    vectorise::UInt<256>       target;

    map.ExpectKeyGetValue(HEADER, header);
    map.ExpectKeyGetValue(TARGET, target);

    p.SetHeader(header);
    p.SetTarget(std::move(target));
  }
};

}  // namespace serializers
}  // namespace fetch
