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

#include "ledger/dag/dag_hash.hpp"

namespace {
using namespace fetch::ledger;

char DAGHashTypeToChar(DAGHash::Type const &t)
{
  switch (t)
  {
  case DAGHash::Type::NODE:
    return 'N';
  case DAGHash::Type::EPOCH:
    return 'E';
  default:
    return 'U';
  }
}
}  // namespace

DAGHash::DAGHash()
  : type{DAGHash::Type::NODE}
{}

DAGHash::DAGHash(DAGHash::Type t)
  : type{t}
{}

DAGHash::DAGHash(DAGHash::ConstByteArray h)
  : hash{std::move(h)}
  , type{DAGHash::Type::NODE}
{}

DAGHash::DAGHash(DAGHash::ConstByteArray h, DAGHash::Type t)
  : hash{std::move(h)}
  , type{t}
{}

DAGHash::operator DAGHash::ConstByteArray()
{
  return hash;
}

DAGHash::operator DAGHash::ConstByteArray() const
{
  return hash;
}

bool DAGHash::empty() const
{
  return hash.empty();
}

bool DAGHash::operator==(DAGHash const &other) const
{
  return hash == other.hash;
}

bool DAGHash::operator>(DAGHash const &rhs) const
{
  return hash > rhs.hash;
}

bool DAGHash::operator<(DAGHash const &rhs) const
{
  return hash < rhs.hash;
}

bool DAGHash::IsEpoch() const
{
  return type == Type::EPOCH;
}

bool DAGHash::IsNode() const
{
  return type == Type::NODE;
}

DAGHash::ConstByteArray DAGHash::ToBase64() const
{
  return hash.ToBase64() + (std::string("/") + DAGHashTypeToChar(type));
}

DAGHash::ConstByteArray DAGHash::ToHex() const
{
  return (hash + (std::string("/") + DAGHashTypeToChar(type))).ToHex();
}
