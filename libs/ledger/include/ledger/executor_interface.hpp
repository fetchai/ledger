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

#include "ledger/chain/digest.hpp"
#include "ledger/execution_result.hpp"

namespace fetch {

class BitVector;

namespace ledger {

class Address;

class ExecutorInterface
{
public:
  using BlockIndex  = uint64_t;
  using SliceIndex  = uint64_t;
  using LaneIndex   = uint32_t;
  using TokenAmount = uint64_t;
  using Status      = ContractExecutionStatus;
  using Result      = ContractExecutionResult;

  // Construction / Destruction
  ExecutorInterface()          = default;
  virtual ~ExecutorInterface() = default;

  /// @name Executor Interface
  /// @{
  virtual Result Execute(Digest const &digest, BlockIndex block, SliceIndex slice,
                         BitVector const &shards)                                              = 0;
  virtual void   SettleFees(Address const &miner, TokenAmount amount, uint32_t log2_num_lanes) = 0;
  /// @}
};

}  // namespace ledger

namespace serializers {

template <typename D>
struct ForwardSerializer<ledger::ExecutorInterface::Status, D>
{
public:
  using Type       = ledger::ExecutorInterface::Status;
  using DriverType = D;

  template <typename Serializer>
  static inline void Serialize(Serializer &s, Type const &status)
  {
    s << static_cast<int32_t>(status);
  }

  template <typename Serializer>
  static inline void Deserialize(Serializer &s, Type &status)
  {
    int32_t raw_status{0};
    s >> raw_status;
    status = static_cast<ledger::ExecutorInterface::Status>(raw_status);
  }
};

template <typename D>
struct MapSerializer<ledger::ExecutorInterface::Result, D>
{
public:
  using Type       = ledger::ExecutorInterface::Result;
  using DriverType = D;

  static uint8_t const STATUS      = 1;
  static uint8_t const CHARGE      = 2;
  static uint8_t const CHARGE_RATE = 3;
  static uint8_t const FEE         = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &result)
  {
    auto map = map_constructor(4);
    map.Append(STATUS, result.status);
    map.Append(CHARGE, result.charge);
    map.Append(CHARGE_RATE, result.charge_rate);
    map.Append(FEE, result.fee);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &result)
  {
    map.ExpectKeyGetValue(STATUS, result.status);
    map.ExpectKeyGetValue(CHARGE, result.charge);
    map.ExpectKeyGetValue(CHARGE_RATE, result.charge_rate);
    map.ExpectKeyGetValue(FEE, result.fee);
  }
};
}  // namespace serializers
}  // namespace fetch
