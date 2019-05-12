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
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class Address : public Object
{
public:
  static constexpr std::size_t RAW_BYTES_SIZE   = 64;
  static constexpr std::size_t STRING_REPR_SIZE = 88;
  using Buffer                                  = std::vector<uint8_t>;
  bool is_address                               = true;

  static Ptr<Address> Constructor(VM *vm, TypeId type_id)
  {
    return new Address{vm, type_id};
  }

  static Ptr<Address> Constructor(VM *vm, TypeId type_id, Ptr<String> const &address)
  {
    return new Address{vm, type_id, address};
  }

  // Construction / Destruction
  Address(VM *vm, TypeId type_id, Ptr<String> const &address = Ptr<String>{},
          bool signed_tx = false)
    : Object(vm, type_id)
    , signed_tx_{signed_tx}
  {
    if (address)
    {
      if (STRING_REPR_SIZE != address->str.size())
      {
        vm->RuntimeError("Incorrectly sized string value for Address value");
        return;
      }

      // decode the base64 address (public key)
      auto const decoded = byte_array::FromBase64(address->str);

      // resize the buffer
      address_.resize(decoded.size());

      // copy the contents of the address
      std::memcpy(address_.data(), decoded.pointer(), decoded.size());
    }
  }

  ~Address() override = default;

  bool HasSignedTx() const
  {
    return signed_tx_;
  }

  void SetSignedTx(bool set = true)
  {
    signed_tx_ = set;
  }

  Buffer const &GetBytes() const
  {
    return address_;
  }

  bool SetBytes(Buffer &&address)
  {
    bool success{false};

    if (RAW_BYTES_SIZE == address.size())
    {
      address_ = std::move(address);
      success  = true;
    }

    return success;
  }

  Ptr<String> AsBase64String()
  {
    return new String{vm_, static_cast<std::string>(byte_array::ToBase64(
                               byte_array::ConstByteArray{address_.data(), address_.size()}))};
  }

  std::vector<uint8_t> ToBytes() const
  {
    return address_;
  }

  void FromBytes(std::vector<uint8_t> &&data)
  {
    if (RAW_BYTES_SIZE != data.size())
    {
      vm_->RuntimeError("Invalid address format");
      return;
    }

    // update the value
    address_ = std::move(data);
  }

private:
  Buffer      address_{};
  std::string string_representation_{"NONE_FOUND"};
  bool        signed_tx_{false};
};

}  // namespace vm
}  // namespace fetch
