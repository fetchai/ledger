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

namespace fetch {
namespace service {

/* A class that defines the context specific parameters being passed to an invoked API func.
 *
 */
class CallContext
{
public:
  using Address = fetch::byte_array::ConstByteArray;

  Address sender_address;
  Address transmitter_address;

  void MarkAsValid()
  {
    is_valid_ = true;
  }

  bool is_valid() const
  {
    return is_valid_;
  }

private:
  bool is_valid_{false};
};

}  // namespace service

namespace serializers {
template <typename D>
struct IgnoredSerializer<service::CallContext, D>
{
public:
  using Type       = service::CallContext;
  using DriverType = D;
};
}  // namespace serializers
}  // namespace fetch
