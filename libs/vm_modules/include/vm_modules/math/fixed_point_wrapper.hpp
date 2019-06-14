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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {

class FixedPointWrapper : public fetch::vm::Object
{
  using FixedPointType = typename fixed_point::FixedPoint<32, 32>;

public:
  FixedPointWrapper()          = delete;
  virtual ~FixedPointWrapper() = default;

  //  static void Bind(vm::Module &module)
  //  {
  //    module.CreateClassType<FixedPointWrapper>("Buffer")
  //        .CreateConstuctor<int32_t>();
  ////        .CreateMemberFunction("copy", &FixedPointWrapper::Copy);
  //  }

  FixedPointWrapper(fetch::vm::VM *vm, fetch::vm::TypeId type_id, double const &val)
    : fetch::vm::Object(vm, type_id)
    , fixed_point_(val)
  {}

  static fetch::vm::Ptr<FixedPointWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                                       double val)
  {
    return {new FixedPointWrapper(vm, type_id, val)};
  }
  //
  //  static fetch::vm::Ptr<FixedPointWrapper> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId
  //  type_id,
  //                                                      FixedPointType fixed_point)
  //  {
  //    return new FixedPointWrapper(vm, type_id, fixed_point);
  //  }
  //
  //  FixedPointType fixed_point()
  //  {
  //    return fixed_point_;
  //  }

  //  bool SerializeTo(vm::ByteArrayBuffer &buffer) override
  //  {
  //    buffer << fixed_point_;
  //    return true;
  //  }
  //
  //  bool DeserializeFrom(vm::ByteArrayBuffer &buffer) override
  //  {
  //    buffer >> fixed_point_;
  //    return true;
  //  }

  double ToDouble() const
  {
    return double(fixed_point_);
  }

private:
  FixedPointType fixed_point_;
};

static void CreateFixedPoint(vm::Module &module)
{
  module.CreateClassType<FixedPointWrapper>("FixedPoint")
      .CreateConstuctor<double>()
      .CreateMemberFunction("double", &FixedPointWrapper::ToDouble);
}

inline void CreateFixedPoint(std::shared_ptr<vm::Module> module)
{
  CreateFixedPoint(*module.get());
}

}  // namespace vm_modules
}  // namespace fetch
