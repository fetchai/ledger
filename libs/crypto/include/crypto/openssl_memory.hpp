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

#include "crypto/openssl_memory_detail.hpp"
#include <memory>

namespace fetch {
namespace crypto {
namespace openssl {
namespace memory {

template <typename T, eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical,
          typename T_Deleter = detail::OpenSSLDeleter<T, P_DeleteStrategy>>
using ossl_unique_ptr = std::unique_ptr<T, T_Deleter>;

template <typename T, eDeleteStrategy P_DeleteStrategy = eDeleteStrategy::canonical,
          typename T_Deleter = detail::OpenSSLDeleter<T, P_DeleteStrategy>>
class OsslSharedPtr : public std::shared_ptr<T>
{
public:
  using Base                                      = std::shared_ptr<T>;
  using Deleter                                   = T_Deleter;
  static constexpr eDeleteStrategy deleteStrategy = P_DeleteStrategy;

  using Base::Base;
  using Base::reset;
  OsslSharedPtr()
    : Base(nullptr)
  {}

  template <class Y>
  explicit OsslSharedPtr(Y *ptr)
    : Base(ptr, Deleter())
  {}

  template <class Y>
  void reset(Y *ptr)
  {
    Base::reset(ptr, Deleter());
  }
};

}  // namespace memory
}  // namespace openssl
}  // namespace crypto
}  // namespace fetch
