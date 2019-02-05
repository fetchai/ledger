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


/**
 * method for creating and int pair
 */

struct IntPair : public fetch::vm::Object
{
  IntPair()          = delete;
  virtual ~IntPair() = default;

  IntPair(fetch::vm::VM *vm, fetch::vm::TypeId type_id, int32_t i, int32_t j)
    : fetch::vm::Object(vm, type_id)
    , first_(i)
    , second_(j)
  {}

  static fetch::vm::Ptr<IntPair> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                             int const &i, int const &j)
  {
    return new IntPair(vm, type_id, i, j);
  }

  int first()
  {
    return first_;
  }

  int second()
  {
    return second_;
  }

private:
  int first_;
  int second_;
};


void CreateIntPair(std::shared_ptr<fetch::vm::Module> module)
{
  module->CreateClassType<IntPair>("IntPair")
    .CreateTypeConstuctor<int, int>()
    .CreateInstanceFunction("first", &IntPair::first)
    .CreateInstanceFunction("second", &IntPair::second);
}


