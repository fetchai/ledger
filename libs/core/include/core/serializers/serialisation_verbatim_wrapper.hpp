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
#include <utility>

namespace fetch {
namespace serializers {

/**
 * @brief Wrapper for (Const)ByteArray instance to serialise its value "AS IS"
 *
 * This class is dedicated to serve as wrapper for (Const)ByteArray instance
 * to serialise its value AS IS (not running it through serialisation process
 * againn), what ultimatelly assumes that its ALREADY IS VALID SERIALISED
 * content.
 * For example, this is useful if there is already pre-serialised content in
 * the (Const)ByteArray instance and it is just necessary to extend or insert
 * it in another serialisation process.
 */
struct Verbatim : public std::reference_wrapper<byte_array::ConstByteArray const>
{
  using Base = std::reference_wrapper<byte_array::ConstByteArray const>;
  using Base::Base;
};

template <typename STREAM>
void Serialize(STREAM &stream, Verbatim const &verbatim)
{
  Verbatim::Base::type &array = verbatim;
  stream.Allocate(array.size());
  stream.WriteBytes(array.pointer(), array.size());
}

}  // namespace serializers
}  // namespace fetch
