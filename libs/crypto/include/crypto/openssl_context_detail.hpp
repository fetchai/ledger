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

#include <openssl/bn.h>

namespace fetch {
namespace crypto {
namespace openssl {
namespace context {
namespace detail {

template <typename T>
using FunctionPtr = void (*)(T *);

// template <typename T>
// struct SessionPrimitive;
//
// template <>
// struct SessionPrimitive<BN_CTX>
//{
//    static const FunctionPtr<BN_CTX> start;
//    static const FunctionPtr<BN_CTX> end;
//};

template <typename T>
struct SessionPrimitive
{
  static const FunctionPtr<T> start;
  static const FunctionPtr<T> end;
};

template <>
const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::start;
template <>
const FunctionPtr<BN_CTX> SessionPrimitive<BN_CTX>::end;

}  // namespace detail
}  // namespace context
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
