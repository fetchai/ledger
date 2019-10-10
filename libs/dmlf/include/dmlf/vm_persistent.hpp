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

#include "vm/io_observer_interface.hpp"

#include "core/byte_array/const_byte_array.hpp"
#include "vm/vm.hpp"

#include <memory>

namespace fetch {
namespace dmlf {

class VmPersistent : public vm::IoObserverInterface
{
public:
  VmPersistent() {}
  VmPersistent(VmPersistent&& other) = default;
  VmPersistent& operator=(VmPersistent&& other) = default; 

  virtual Status Read(const std::string &key, void *data, uint64_t &size) override;
  virtual Status Write(const std::string &key, void const *data, uint64_t size) override;
  virtual Status Exists(const std::string &key) override;

  virtual VmPersistent DeepCopy() const;

protected:
private:
  using Buffer = fetch::byte_array::ConstByteArray;
  using Store =  std::map<std::string, Buffer>;

  Store store_;

  VmPersistent(const VmPersistent &other) = delete;
  VmPersistent &operator=(const VmPersistent &other) = delete;
  bool operator==(const VmPersistent &other) = delete;
  bool operator<(const VmPersistent &other) = delete;
};

} // namespace dmlf
} // namespace fetch
