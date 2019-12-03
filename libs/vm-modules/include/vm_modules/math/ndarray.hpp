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
#include "vm/variant.hpp"

namespace fetch {

namespace math {
template <typename, typename>
class Tensor;
}
namespace vm {
class Module;
template <typename T>
struct Array;
}  // namespace vm

namespace vm_modules {
namespace math {

using namespace fetch::vm;  // DELETEME!

class ITensor : public fetch::vm::Object
{
public:
  ITensor()           = delete;
  ~ITensor() override = default;
  using Index         = fetch::math::SizeType;
  // static Ptr<ITensor> Constructor(VM *vm, TypeId type_id, int32_t num_rows, int32_t num_columns);
  static Ptr<ITensor> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::Array<fetch::math::SizeType>> const &shape);

  virtual TemplateParameter1 At(AnyInteger const &idx1)                               = 0;
  virtual TemplateParameter1 At(AnyInteger const &idx1, AnyInteger const &idx2) const = 0;
  virtual TemplateParameter1 At(AnyInteger const &idx1, AnyInteger const &idx2,
                                AnyInteger const &idx3) const                         = 0;
  virtual TemplateParameter1 At(AnyInteger const &idx1, AnyInteger const &idx2,
                                AnyInteger const &idx3, AnyInteger const &idx4) const = 0;

  virtual TemplateParameter1 GetIndexedValue(AnyInteger const &row, AnyInteger const &column) = 0;
  virtual void               SetIndexedValue(AnyInteger const &row, AnyInteger const &column,
                                             TemplateParameter1 const &value)                 = 0;

  static void Bind(fetch::vm::Module &module);

protected:
  ITensor(VM *vm, TypeId type_id);
};

template <typename T>
struct NDArray : public ITensor
{
  NDArray()           = delete;
  ~NDArray() override = default;

  // TODO(VH): make me a template with variable shape ints
  explicit NDArray(VM *vm, TypeId type_id, TypeId element_type_id__, std::size_t num_rows,
                   std::size_t num_columns);
  explicit NDArray(fetch::vm::VM *vm, fetch::vm::TypeId type_id,
                   const std::vector<fetch::math::SizeType> &shape);

  using TensorType = typename fetch::math::Tensor<T>;

  explicit NDArray(fetch::vm::VM *vm, fetch::vm::TypeId type_id, TensorType tensor);

  explicit NDArray(fetch::vm::VM *vm, fetch::vm::TypeId type_id);

  static fetch::vm::Ptr<NDArray> Constructor(
      fetch::vm::VM *vm, fetch::vm::TypeId type_id,
      fetch::vm::Ptr<fetch::vm::Array<fetch::math::SizeType>> const &shape);

  // TODO(VH): removeme
  static Ptr<NDArray> AcquireMatrix(VM *vm, TypeId type_id, TypeId element_type_id,
                                    std::size_t num_rows, std::size_t num_columns);

  static void Bind(fetch::vm::Module &module);

  ////////////////////////////////////
  /// ACCESSING AND SETTING VALUES ///
  ////////////////////////////////////
  TemplateParameter1 At(AnyInteger const &idx1) override;
  TemplateParameter1 At(AnyInteger const &idx1, AnyInteger const &idx2) const override;
  TemplateParameter1 At(AnyInteger const &idx1, AnyInteger const &idx2,
                        AnyInteger const &idx3) const override;
  TemplateParameter1 At(AnyInteger const &idx1, AnyInteger const &idx2, AnyInteger const &idx3,
                        AnyInteger const &idx4) const override;

  void SetIndexedValue(AnyInteger const &row, AnyInteger const &column,
                       TemplateParameter1 const &value) override;

  TemplateParameter1 GetIndexedValue(AnyInteger const &row, AnyInteger const &column) override;

  void Copy(TensorType const &other);

  void Fill(T const &value);

  void FillRandom();

  void Negate(Ptr<Object> &object) override;

  void Add(Ptr<Object> &lhso, Ptr<Object> &rhso) override;

  void RightAdd(Variant &objectv, Variant &rhsv) override;

  void InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;

  void InplaceRightAdd(Ptr<Object> const &lhso, Variant const &rhsv) override;

  void Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso) override;

  void RightSubtract(Variant &objectv, Variant &rhsv) override;

  void InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;

  void InplaceRightSubtract(Ptr<Object> const &lhso, Variant const &rhsv) override;

  void Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso) override;

  void LeftMultiply(Variant &lhsv, Variant &objectv) override;

  void RightMultiply(Variant &objectv, Variant &rhsv) override;

  void InplaceRightMultiply(Ptr<Object> const &lhso, Variant const &rhsv) override;

  void RightDivide(Variant &objectv, Variant &rhsv) override;

  void InplaceRightDivide(Ptr<Object> const &lhso, Variant const &rhsv) override;

  T *Find(AnyInteger const &row, AnyInteger const &column);

  // TemplateParameter1 At(AnyInteger const &row, AnyInteger const &column) override;

  fetch::math::Tensor<T> tensor_;
  TypeId                 element_type_id_;
};

}  // namespace math
}  // namespace vm_modules
}  // namespace fetch
