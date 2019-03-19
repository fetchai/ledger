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

#include <functional>
#include <memory>
#include <unordered_set>

namespace fetch {
namespace ledger {

class Identifier;
class Contract;
class StorageInterface;

class ChainCodeFactory
{
public:
  using ConstByteArray  = byte_array::ConstByteArray;
  using ContractPtr     = std::shared_ptr<Contract>;
  using ContractNameSet = std::unordered_set<ConstByteArray>;

  ContractPtr Create(Identifier const &name, StorageInterface &storage) const;

  ContractNameSet const &GetChainCodeContracts() const;
};

}  // namespace ledger
}  // namespace fetch
