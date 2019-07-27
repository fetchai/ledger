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

#include "math/tensor.hpp"
#include "vm/object.hpp"
#include "vm_modules/math/type.hpp"

#include <cstdint>
#include <vector>

namespace fetch {

namespace vm {
class Module;
template <typename T>
struct Array;
}  // namespace vm

namespace vm_modules {
namespace math {

class VMTensor : public fetch::vm::Object
{

public:
  using DataType   = fetch::vm_modules::math::DataType;
  using ArrayType  = fetch::math::Tensor<DataType>;
  using SizeType   = ArrayType::SizeType;
  using SizeVector = ArrayType::SizeVector;

  VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, std::vector<std::uint64_t> const &shape);

  VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id, ArrayType tensor);

  VMTensor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<VMTensor> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                                              fetch::vm::Ptr<fetch::vm::Array<SizeType>> shape);

  static fetch::vm::Ptr<VMTensor> Constructor(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static void Bind(fetch::vm::Module &module);

  SizeVector shape() const;

  SizeType size() const;

  ////////////////////////////////////
  /// ACCESSING AND SETTING VALUES ///
  ////////////////////////////////////

  DataType AtOne(SizeType idx1) const;

  DataType AtTwo(uint64_t idx1, uint64_t idx2) const;

  DataType AtThree(uint64_t idx1, uint64_t idx2, uint64_t idx3) const;

  void SetAt(uint64_t index, DataType value);

  void Copy(ArrayType const &other);

  void Fill(DataType const &value);

  void FillRandom();

  bool Reshape(fetch::vm::Ptr<fetch::vm::Array<SizeType>> const &new_shape);

  //////////////////////////////
  /// PRINTING AND EXPORTING ///
  //////////////////////////////

  fetch::vm::Ptr<fetch::vm::String> ToString() const;

  ArrayType &GetTensor();

  ArrayType const &GetConstTensor();

  bool SerializeTo(serializers::ByteArrayBuffer &buffer) override;

  bool DeserializeFrom(serializers::ByteArrayBuffer &buffer) override;

private:
  ArrayType tensor_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
