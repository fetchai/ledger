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

#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class Address : public Object
{
public:
  static constexpr std::size_t SIZE = 64;
  static constexpr std::size_t STRING_REPR_SIZE = 88;
  using Buffer                      = std::vector<uint8_t>;
  //using is_address = bool;
  bool is_address = true;

  static Ptr<Address> Constructor(VM *vm, TypeId id)
  {
    return new Address{vm, id};
  }

  // Construction / Destruction
  Address(VM *vm, TypeId id, bool signed_tx = false)
    : Object(vm, id)
    , signed_tx_{signed_tx}
    , vm_{vm}
  {}
  ~Address() override = default;

  bool HasSignedTx() const
  {
    return signed_tx_;
  }

  void SetSignedTx(bool set = true)
  {
    signed_tx_ = true;
  }

  Buffer const &GetBytes() const
  {
    return address_;
  }

  bool SetBytes(Buffer &&address)
  {
    bool success{false};

    if (SIZE == address.size())
    {
      address_ = std::move(address);
      success  = true;
    }

    return success;
  }

  void SetStringRepresentation(std::string const &repr)
  {
    if(repr.size() != STRING_REPR_SIZE)
    {
      std::cerr << "Expected string size of "<< STRING_REPR_SIZE <<", got: " << repr.size() << std::endl;
    }

    string_representation_ = repr;
  }

  fetch::vm::Ptr<fetch::vm::String> AsString()
  {
    //std::string copy_buffer{address_.begin(), address_.end()};
    //fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm_, copy_buffer));
    fetch::vm::Ptr<fetch::vm::String> ret(new fetch::vm::String(vm_, string_representation_));
    return ret;
  }

  uint64_t AsBytesSize() const
  {
    return STRING_REPR_SIZE + SIZE;
  }

  void *AsBytes()
  {
    void *bytes = malloc(STRING_REPR_SIZE + SIZE);

    std::cerr << "sizes:" << std::endl;
    std::cerr << address_.size() << std::endl;
    std::cerr << string_representation_.size() << std::endl;

    memcpy(bytes, address_.data(), address_.size());
    memcpy((uint8_t *)bytes + SIZE, string_representation_.c_str(), string_representation_.size());
    return bytes;
  }

  void FromBytes(void *data)
  {
    address_ = Buffer((uint8_t *)data, (uint8_t *)data + SIZE);
    string_representation_.assign(((const char *)data)+SIZE, (std::size_t)STRING_REPR_SIZE);
  }

private:
  Buffer address_{};
  std::string string_representation_{"NONE_FOUND"};
  bool   signed_tx_{false};
  VM *vm_;
};

}  // namespace vm
}  // namespace fetch
