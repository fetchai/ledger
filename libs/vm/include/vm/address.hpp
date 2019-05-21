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
#include "core/serializers/byte_array.hpp"
#include "ledger/chain/v2/address.hpp"

#include "vm/vm.hpp"

namespace fetch {
namespace vm {

class Address : public Object
{
public:
  static constexpr std::size_t RAW_BYTES_SIZE = 32;

  using Buffer = std::vector<uint8_t>;

  static Ptr<Address> Constructor(VM *vm, TypeId type_id)
  {
    return new Address{vm, type_id};
  }

  static Ptr<Address> Constructor(VM *vm, TypeId type_id, Ptr<String> const &address)
  {
    return new Address{vm, type_id, address};
  }

  // Construction / Destruction
  Address(VM *vm, TypeId id, Ptr<String> const &address = Ptr<String>{}, bool signed_tx = false)
    : Object(vm, id)
    , signed_tx_{signed_tx}
    , vm_{vm}
  {
    if (address && !ledger::v2::Address::Parse(address->str.c_str(), address_))
    {
      vm->RuntimeError("Unable to parse address");
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

  Ptr<String> AsString()
  {
    return new String{vm_, std::string{address_.display()}};
  }

  std::vector<uint8_t> ToBytes() const
  {
    auto const &address = address_.address();
    auto const *start   = address.pointer();
    auto const *end     = start + address.size();

    return {start, end};
  }

  void FromBytes(std::vector<uint8_t> &&data)
  {
    if (RAW_BYTES_SIZE != data.size())
    {
      vm_->RuntimeError("Invalid address format");
      return;
    }

    // update the value
    address_ = ledger::v2::Address({data.data(), data.size()});
  }

  ledger::v2::Address const &address() const
  {
    return address_;
  }

  Address &operator=(ledger::v2::Address const &address)
  {
    address_ = address;
    return *this;
  }

  bool operator==(ledger::v2::Address const &other) const
  {
    return address_ == other;
  }

  bool SerializeTo(ByteArrayBuffer &buffer) override
  {
    buffer << address_.address();
    return true;
  }

  bool DeserializeFrom(ByteArrayBuffer &buffer) override
  {
    fetch::byte_array::ConstByteArray raw_address{};
    buffer >> raw_address;
    address_ = ledger::v2::Address{raw_address};
    return true;
  }

private:
  ledger::v2::Address address_;
  bool                signed_tx_{false};
  VM *                vm_;
};

}  // namespace vm
}  // namespace fetch
