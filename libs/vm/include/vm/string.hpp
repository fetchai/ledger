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

#include "vm/defs.hpp"

namespace fetch {
namespace vm {

struct String : public Object
{
  String()          = delete;
  virtual ~String() = default;

  String(VM *vm, std::string str__, bool is_literal__ = false)
    : Object(vm, TypeIds::String)
    , str(std::move(str__))
    , is_literal(is_literal__)
  {}

  virtual size_t GetHashCode() override;
  virtual bool   IsEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  virtual bool   IsNotEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  virtual bool   IsLessThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  virtual bool   IsLessThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  virtual bool   IsGreaterThan(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  virtual bool   IsGreaterThanOrEqual(Ptr<Object> const &lhso, Ptr<Object> const &rhso) override;
  virtual void   Add(Ptr<Object> &lhso, Ptr<Object> &rhso) override;
  std::string    str;
  bool           is_literal;
};

}  // namespace vm
}  // namespace fetch
