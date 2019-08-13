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
#include <iostream>

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

  template <typename Y, std::size_t M>
  VectorRegisterIterator(VectorRegisterIterator<Y, M> &o, std::size_t const offset)
    : ptr_(reinterpret_cast<mm_register_type *>(o.pointer()))
    , end_(reinterpret_cast<mm_register_type *>(o.end()))
  {
    ptr_ += offset;
    std::cout << "ptr_ = " << std::hex << ptrdiff_t(ptr_) << std::endl;
    std::cout << "end_ = " << std::hex << ptrdiff_t(end_) << std::endl;
  }

  VectorRegisterIterator(type const *d, std::size_t size)
    : ptr_((mm_register_type *)d)
    , end_((mm_register_type *)(d + size))
  {
    std::cout << "ptr_ = " << std::hex << ptrdiff_t(ptr_) << std::endl;
    std::cout << "end_ = " << std::hex << ptrdiff_t(end_) << std::endl;
  }

  void Next(VectorRegisterType &m)
  {
    assert((end_ == nullptr) || (ptr_ < end_));
    std::cout << "ptr_ = " << std::hex << ptrdiff_t(ptr_) << std::endl;

    m.data() = *ptr_;
    ++ptr_;
    std::cout << "ptr_ = " << std::hex << ptrdiff_t(ptr_) << std::endl;
  }

  mm_register_type *pointer() const
  {
    return ptr_;
  }

  mm_register_type *end() const
  {
    return end_;
  }
private:
  mm_register_type *ptr_;
  mm_register_type *end_;
};
}  // namespace vectorise
}  // namespace fetch
