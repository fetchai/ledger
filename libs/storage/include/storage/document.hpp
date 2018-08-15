#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
namespace storage {

struct Document
{
  explicit operator byte_array::ConstByteArray() { return document; }

  byte_array::ByteArray document;
  bool                  was_created = false;
  bool                  failed      = false;
};

template <typename T>
void Serialize(T &serializer, Document const &b)
{
  serializer << b.document << b.was_created << b.failed;
}

template <typename T>
void Deserialize(T &serializer, Document &b)
{
  serializer >> b.document >> b.was_created >> b.failed;
}

}  // namespace storage
}  // namespace fetch
