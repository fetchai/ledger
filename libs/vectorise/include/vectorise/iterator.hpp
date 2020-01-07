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
  using MMRegisterType     = typename VectorRegisterType::MMRegisterType;

  VectorRegisterIterator()
    : ptr_(nullptr)
    , end_(nullptr)
  {}

  template <typename Y, std::size_t M>
  VectorRegisterIterator(VectorRegisterIterator<Y, M> &o, std::size_t const offset)
    : ptr_(reinterpret_cast<MMRegisterType *>(o.pointer()))
    , end_(reinterpret_cast<MMRegisterType *>(o.end()))
  {
    ptr_ += offset;
  }

  VectorRegisterIterator(type const *d, std::size_t size)
    : ptr_(reinterpret_cast<MMRegisterType const *>(d))
    , end_(reinterpret_cast<MMRegisterType const *>(d + size))
  {}

  void Next(VectorRegisterType &m)
  {
    assert((end_ == nullptr) || (ptr_ < end_));

    m.data() = *ptr_;
    ++ptr_;
  }

  MMRegisterType const *pointer() const
  {
    return ptr_;
  }

  MMRegisterType const *end() const
  {
    return end_;
  }

private:
  MMRegisterType const *ptr_;
  MMRegisterType const *end_;
};

}  // namespace vectorise
}  // namespace fetch
