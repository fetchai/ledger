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

#include "vm/fixed.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace fetch {
namespace vm {

Fixed128::Fixed128(VM *vm, fixed_point::fp128_t const &data)
  : Object(vm, TypeIds::Fixed128)
  , data_{data}
{}

Fixed128::Fixed128(vm::VM *vm, vm::TypeId /*type_id*/, fixed_point::fp128_t const &data)
  : Object(vm, TypeIds::Fixed128)
  , data_{data}
{}

Fixed128::Fixed128(vm::VM *vm, vm::TypeId /*type_id*/, byte_array::ByteArray const &data)
  : Object(vm, TypeIds::Fixed128)
  , data_{*reinterpret_cast<int128_t const *>(data.pointer())}
{}

Ptr<Fixed128> Fixed128::Copy() const
{
  return Ptr<Fixed128>{new Fixed128{this->vm_, this->data_}};
}

bool Fixed128::IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data_ == rhs->data_;
}

bool Fixed128::IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data_ != rhs->data_;
}

bool Fixed128::IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data_ < rhs->data_;
}

bool Fixed128::IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data_ <= rhs->data_;
}

bool Fixed128::IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data_ > rhs->data_;
}

bool Fixed128::IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  return lhs->data_ >= rhs->data_;
}

void Fixed128::Add(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  if (lhs->IsTemporary())
  {
    lhs->data_ += rhs->data_;
    return;
  }
  if (rhs->IsTemporary())
  {
    rhs->data_ += lhs->data_;
    lhso = std::move(rhs);
    return;
  }
  Ptr<Fixed128> n(new Fixed128(vm_, lhs->data_ + rhs->data_));
  lhso = std::move(n);
}

void Fixed128::InplaceAdd(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  lhs->data_ += rhs->data_;
}

void Fixed128::Subtract(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  if (lhs->IsTemporary())
  {
    lhs->data_ -= rhs->data_;
    return;
  }
  if (rhs->IsTemporary())
  {
    rhs->data_ -= lhs->data_;
    lhso = std::move(rhs);
    return;
  }
  Ptr<Fixed128> n(new Fixed128(vm_, lhs->data_ - rhs->data_));
  lhso = std::move(n);
}

void Fixed128::InplaceSubtract(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  lhs->data_ -= rhs->data_;
}

void Fixed128::Multiply(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  if (lhs->IsTemporary())
  {
    lhs->data_ *= rhs->data_;
    return;
  }
  if (rhs->IsTemporary())
  {
    rhs->data_ *= lhs->data_;
    lhso = std::move(rhs);
    return;
  }
  Ptr<Fixed128> n(new Fixed128(vm_, lhs->data_ * rhs->data_));
  lhso = std::move(n);
}

void Fixed128::InplaceMultiply(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  lhs->data_ *= rhs->data_;
}

void Fixed128::Divide(Ptr<Object> &lhso, Ptr<Object> &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  if (lhs->IsTemporary())
  {
    lhs->data_ /= rhs->data_;
    return;
  }
  if (rhs->IsTemporary())
  {
    rhs->data_ /= lhs->data_;
    lhso = std::move(rhs);
    return;
  }
  Ptr<Fixed128> n(new Fixed128(vm_, lhs->data_ / rhs->data_));
  lhso = std::move(n);
}

void Fixed128::InplaceDivide(Ptr<Object> const &lhso, Ptr<Object> const &rhso)
{
  Ptr<Fixed128> lhs = lhso;
  Ptr<Fixed128> rhs = rhso;
  lhs->data_ /= rhs->data_;
}

void Fixed128::Negate(Ptr<Object> &object)
{
  Ptr<Fixed128> fixed = object;
  Ptr<Fixed128> n(new Fixed128(vm_, -fixed->data_));
  object = std::move(n);
}

ChargeAmount Fixed128::IsEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                              Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::IsNotEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                 Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::IsLessThanChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                 Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::IsLessThanOrEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                        Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::IsGreaterThanChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                    Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::IsGreaterThanOrEqualChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                           Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::AddChargeEstimator(Ptr<Object> const & /*lhso*/,
                                          Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::InplaceAddChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                 Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::SubtractChargeEstimator(Ptr<Object> const & /*lhso*/,
                                               Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::InplaceSubtractChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                      Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::MultiplyChargeEstimator(Ptr<Object> const & /*lhso*/,
                                               Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::InplaceMultiplyChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                      Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::DivideChargeEstimator(Ptr<Object> const & /*lhso*/,
                                             Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::InplaceDivideChargeEstimator(Ptr<Object> const & /*lhso*/,
                                                    Ptr<Object> const & /*rhso*/)
{
  return 1;
}

ChargeAmount Fixed128::NegateChargeEstimator(Ptr<Object> const & /*object*/)
{
  return 1;
}

bool Fixed128::SerializeTo(MsgPackSerializer &buffer)
{
  buffer << data_;
  return true;
}

bool Fixed128::DeserializeFrom(MsgPackSerializer &buffer)
{
  buffer >> data_;
  return true;
}

}  // namespace vm
}  // namespace fetch
