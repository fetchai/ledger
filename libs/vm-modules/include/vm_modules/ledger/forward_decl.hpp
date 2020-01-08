#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "vm/array.hpp"
#include "vm/object.hpp"

namespace fetch {

namespace vm {
class Module;
class Address;
class IArray;
}  // namespace vm

namespace vm_modules {

namespace math {
class UInt256Wrapper;
}  // namespace math

namespace ledger {

class Transfer;
class Block;
class Context;
class Transaction;

using AddressPtr = vm::Ptr<vm::Address>;

using DigestPtr      = vm::Ptr<math::UInt256Wrapper>;
using StringPtr      = vm::Ptr<vm::String>;
using TransferPtr    = vm::Ptr<Transfer>;
using TransactionPtr = vm::Ptr<Transaction>;
using BlockPtr       = vm::Ptr<Block>;
using ContextPtr     = vm::Ptr<Context>;

using NativeTokenAmount = uint64_t;
using BlockIndex        = uint64_t;

using AddressesPtr = vm::Ptr<vm::Array<AddressPtr>>;
using TransfersPtr = vm::Ptr<vm::Array<TransferPtr>>;

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
