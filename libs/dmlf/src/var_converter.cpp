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
#include "core/serializers/array_interface.hpp"
#include "core/serializers/container_constructor_interface.hpp"
#include "dmlf/var_converter.hpp"

namespace fetch {
namespace dmlf {

std::string describe(const variant::Variant &x)
{
  if (x.IsUndefined())
  {
    return "undef";
  }
  if (x.IsInteger())
  {
    return "integer";
  }
  if (x.IsFloatingPoint())
  {
    return "float(" + std::to_string(x.As<double>()) + ")";
  }
  if (x.IsFixedPoint())
  {
    return "fp";
  }
  if (x.IsBoolean())
  {
    return std::string("bool(") + (x.As<bool>() ? "true" : "false") + ")";
  }
  if (x.IsString())
  {
    return "string(" + x.As<std::string>() + ")";
  }
  if (x.IsNull())
  {
    return "null";
  }
  if (x.IsArray())
  {
    return "array[" + std::to_string(x.size()) + "]";
  }
  if (x.IsObject())
  {
    return "object{" + std::to_string(x.size()) + "}";
  }
  return "unknown";
}

bool VarConverter::Convert(byte_array::ByteArray &target, const variant::Variant &source,
                           const std::string &format)
{
  // std::cout << "START VarConverter::Convert(" << format << ")" << std::endl;
  fetch::serializers::MsgPackSerializer ss;

  if (Convert(ss, source, format))
  {
    auto data = ss.data().pointer();
    auto size = ss.size();
    target.Reserve(size);
    target.Resize(size);
    target.WriteBytes(data, size);
    // std::cout << "start VarConverter::Convert() OK!!" << std::endl;
    return true;
  }

  // std::cout << "start VarConverter::Convert() **FAIL**" << std::endl;
  return false;
}

bool VarConverter::Convert(fetch::serializers::MsgPackSerializer &os,
                           const variant::Variant &source, const std::string &format)
{
  // std::cout << "VarConverter::Convert(" << describe(source) << " => " << format << ")" <<
  // std::endl;
  if (format == "Int64")
  {
    if (!source.IsInteger())
    {
      std::cout << "Want int64, L::VAR is not int" << std::endl;
      return false;
    }
    os << source.As<int64_t>();
    return true;
  }

  if (format == "Int32")
  {
    if (!source.IsInteger())
    {
      std::cout << "Want int32, L::VAR is not int" << std::endl;
      return false;
    }
    os << static_cast<int32_t>(source.As<int64_t>());
    return true;
  }

  if (format.substr(0, 6) == "Array<")
  {
    if (!source.IsArray())
    {
      std::cout << "Want array, L::VAR is not array" << std::endl;
      return false;
    }

    os << format;
    os << uint32_t(source.size());

    auto subformat = format.substr(6, format.size() - 7);
    // std::cout << "array of.... " << subformat << std::endl;
    bool r = true;
    for (std::size_t i = 0; i < source.size(); i++)
    {
      r &= Convert(os, source[i], subformat);
    }
    return r;
  }

  std::cout << "Want " << format << " and can't get it." << std::endl;
  return false;

  /*
  if (source.IsArray() )
  {
    using fetch::serializers::TypeCodes;
    using fetch::serializers::MsgPackSerializer;
    using fetch::serializers::interfaces::ArrayInterface;
    using fetch::serializers::interfaces::ContainerConstructorInterface;
    using Constructor = ContainerConstructorInterface<
      MsgPackSerializer,
      ArrayInterface<MsgPackSerializer>,
      serializers::TypeCodes::ARRAY_CODE_FIXED,
      serializers::TypeCodes::ARRAY_CODE16,
      serializers::TypeCodes::ARRAY_CODE32
    >;

    os << std::string("Array<Int64x>");
    os << uint32_t(source.size());

    Constructor constructor(os);
    //constructor(source.size());
    bool r = true;
    for (std::size_t i = 0; i < source.size(); i++)
    {
      r &= Convert(os, source[i]);
    }
    return r;
  }
  if (source.IsObject() )
  {
    return false;
  }
  return false;
  */
}

void VarConverter::Dump(byte_array::ByteArray &ba)
{
  std::size_t width = 40;

  for (std::size_t orig = 0; orig < ba.size(); orig += width)
  {
    for (std::size_t offs = 0; offs < width && orig + offs < ba.size(); offs++)
    {
      auto byte = ba[orig + offs];
      std::cout << "  " << std::hex << std::setw(2) << int(byte);
    }
    std::cout << std::endl;
    for (std::size_t offs = 0; offs < width && orig + offs < ba.size(); offs++)
    {
      auto byte = ba[orig + offs];
      std::cout << " " << std::dec << std::setw(3) << int(byte);
    }
    std::cout << std::endl;
    for (std::size_t offs = 0; offs < width && orig + offs < ba.size(); offs++)
    {
      auto byte = ba[orig + offs];
      auto c    = static_cast<char>(byte);
      if (byte < 32 || byte >= 127)
      {
        c = '.';
      }
      std::cout << "   " << std::dec << c;
    }
    std::cout << std::endl;
    std::cout << std::endl;
  }
}

}  // namespace dmlf
}  // namespace fetch
