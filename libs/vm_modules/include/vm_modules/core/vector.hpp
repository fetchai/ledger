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

namespace fetch {
namespace vm_modules {

/**
 * method for printing string to std::cout
 */

template <typename T>
struct Vector : public fetch::vm::Object
{
  Vector()           = delete;
  virtual ~Vector()  = default;

  Vector(fetch::vm::VM *vm, fetch::vm::TypeId type_id, int32_t size) : fetch::vm::Object(vm, type_id),
    vector_(static_cast<std::size_t>(size))
  {
  }

  static fetch::vm::Ptr<Vector> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, int32_t size)
  {
    return new Vector(vm, type_id, size);
  }

  std::uint32_t size()
  {
    return std::uint32_t(vector_.size());
  }

  void resize(std::uint32_t size)
  {
    vector_.resize(std::size_t(size));
  }

  T operator[](int32_t idx)
  {
    return vector_[std::size_t(idx)];
  }

  T operator[](int32_t idx) const
  {
    return vector_[std::size_t(idx)];
  }

  T& at(int32_t idx)
  {
    return vector_.at(idx);
  }

  T& at(int32_t idx) const
  {
    return vector_.at(idx);
  }

  T& front()
  {
    return vector_.front();
  }

  T& front() const
  {
    return vector_.front();
  }

  T& back()
  {
    return vector_.back();
  }

  T& back() const
  {
    return vector_.back();
  }

  void emplace_back(T val)
  {
    vector_.emplace_back(val);
  }

  void push_back(T val)
  {
    vector_.push_back(val);
  }

  void clear()
  {
    vector_.clear();
  }

  void pop_back()
  {
    vector_.pop_back();
  }

  T* data() noexcept
  {
    return vector_.data();
  }

  const T* data() noexcept
  {
    return vector_.data();
  }
  

private:
  std::vector<T> vector_;
};


template <typename T>
void CreateVectorImpl(fetch::vm::Module &module)
{
  module.CreateClassType<Vector<T>>("Vector")
          .template CreateTypeConstuctor<int32_t>()
          .CreateInstanceFunction("size", &Vector<T>::size)
          .CreateInstanceFunction("at", &Vector<T>::operator[]);
}

void CreateVector(fetch::vm::Module &module)
{
  CreateVectorImpl<int32_t>(module);
  CreateVectorImpl<uint32_t>(module);
  CreateVectorImpl<float_t>(module);
  CreateVectorImpl<double_t>(module);
}

}  // namespace vm_modules
}  // namespace fetch
