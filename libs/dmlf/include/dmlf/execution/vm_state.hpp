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

#include "vm/io_observer_interface.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "vm/vm.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace dmlf {

class VmState : public vm::IoObserverInterface
{
public:
  VmState()                         = default;
  VmState(VmState &&other) noexcept = default;
  VmState &operator=(VmState &&other) noexcept = default;

  VmState(VmState const &other) = delete;
  VmState &operator=(VmState const &other) = delete;

  Status Read(std::string const &key, void *data, uint64_t &size) override;
  Status Write(std::string const &key, void const *data, uint64_t size) override;
  Status Exists(std::string const &key) override;

  VmState DeepCopy() const;

private:
  using Buffer = fetch::byte_array::ConstByteArray;
  using Store  = std::unordered_map<std::string, Buffer>;

  Store store_;
};

}  // namespace dmlf
}  // namespace fetch
