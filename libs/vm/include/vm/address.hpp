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
  using Buffer                      = std::vector<uint8_t>;

  static Ptr<Address> Constructor(VM *vm, TypeId id)
  {
    return new Address{vm, id};
  }

  // Construction / Destruction
  Address(VM *vm, TypeId id, bool signed_tx = false)
    : Object(vm, id)
    , signed_tx_{signed_tx}
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

private:
  Buffer address_{};
  bool   signed_tx_{false};
};

}  // namespace vm
}  // namespace fetch