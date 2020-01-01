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

#include "core/byte_array/byte_array.hpp"
#include "variant/variant.hpp"

namespace fetch {

namespace serializers {
class MsgPackSerializer;
}

namespace dmlf {

class VarConverter
{
public:
  VarConverter()          = default;
  virtual ~VarConverter() = default;

  bool Convert(byte_array::ByteArray &target, const variant::Variant &source,
               const std::string &format);

  VarConverter(VarConverter const &other) = delete;
  VarConverter &operator=(VarConverter const &other)  = delete;
  bool          operator==(VarConverter const &other) = delete;
  bool          operator<(VarConverter const &other)  = delete;

  void Dump(byte_array::ByteArray &ba);

protected:
private:
  bool Convert(fetch::serializers::MsgPackSerializer &os, const variant::Variant &source,
               const std::string &format);
};

}  // namespace dmlf
}  // namespace fetch
