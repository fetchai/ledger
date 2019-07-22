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

#include "vectorise/register.hpp"

#include <cassert>

namespace fetch {
namespace vectorise {

template <typename T, std::size_t N = sizeof(T)>
class VectorRegisterIterator
{
public:
  using type               = T;
  using VectorRegisterType = VectorRegister<T, N>;
  using mm_register_type   = typename VectorRegisterType::mm_register_type;

  VectorRegisterIterator()
    : ptr_(nullptr)
    , end_(nullptr)
  {}
  VectorRegisterIterator(type const *d, std::size_t size)
    : ptr_((mm_register_type *)d)
    , end_((mm_register_type *)(d + size))
  {}

  void Next(VectorRegisterType &m)
  {
    assert((end_ == nullptr) || (ptr_ < end_));

    m.data() = *ptr_;
    ++ptr_;
  }

private:
  mm_register_type *ptr_;
  mm_register_type *end_;
};
}  // namespace vectorise
}  // namespace fetch
