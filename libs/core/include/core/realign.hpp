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

#include <cstring>
#include <memory>

namespace fetch {
namespace memory {

template <class T>
T const *Realign(void const *buf, std::size_t amount)
{
  static constexpr std::size_t size = sizeof(T);

  {
    std::size_t bufsize = amount * size;
    void *      tmp     = const_cast<void *>(buf);

    auto realigned_ptr = std::align(alignof(T), sizeof(T), tmp, bufsize);
    if (realigned_ptr == buf)
    {
      return static_cast<T const *>(buf);
    }
  }

  auto ret_val = new T[amount];

  std::memcpy(ret_val, buf, size * amount);
  return const_cast<T const *>(ret_val);
}

template <class T>
T *Realign(void *buf, std::size_t amount)
{
  return const_cast<T *>(Realign<T>(const_cast<void const *>(buf), amount));
}

}  // namespace memory
}  // namespace fetch
