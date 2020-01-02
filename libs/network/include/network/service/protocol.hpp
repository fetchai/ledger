#pragma once
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

#include "core/assert.hpp"
#include "core/mutex.hpp"
#include "network/management/abstract_connection.hpp"
#include "network/service/callable_class_member.hpp"
#include "network/service/error_codes.hpp"
#include "network/service/types.hpp"

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace fetch {
namespace service {

/**
 * A class that defines a generic protocol.
 *
 * This class is used for defining a general protocol with
 * remote-function-calls (SERVICEs) and data feeds. The SERVICEs are defined
 * from a C++ function signature using any sub class of
 * <service::AbstractCallable> including <service::Function> and
 * <service::CallableClassMember>. The feeds are available from any
 * functionality implementation that sub-classes
 * <service::HasPublicationFeed>.
 *
 * A current limitation of the implementation is that there is only
 * support for 256 SERVICE functions. It the next version of this class,
 * this should be changed to be variable and allocated at construction
 * time.
 *
 * TODO(issue 21):
 */
class Protocol
{
public:
  using CallableType         = AbstractCallable *;
  using StoredType           = std::shared_ptr<AbstractCallable>;
  using ByteArrayType        = byte_array::ConstByteArray;
  using ConnectionHandleType = typename network::AbstractConnection::ConnectionHandleType;
  using MiddlewareType =
      std::function<void(ConnectionHandleType const &, byte_array::ByteArray const &)>;

  Protocol()          = default;
  virtual ~Protocol() = default;

  /* Operator to access the different functions in the protocol.
   * @n is the idnex of callable in the protocol.
   *
   * The result of this operator is a <CallableType> that can be
   * invoked in accodance with the definition of an
   * <service::AbstractCallable>.
   *
   * This operator throws a <SerializableException> if the index does
   * not map to a callable
   *
   * @return a reference to the call.
   */
  CallableType operator[](FunctionHandlerType const &n)
  {
    auto iter = members_.find(n);
    if (iter == members_.end())
    {
      DumpMemberTable();

      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to look up function handler: ", n);

      throw serializers::SerializableException(
          error::MEMBER_NOT_FOUND, ByteArrayType("Could not find protocol member function"));
    }
    return iter->second.get();
  }

  /* Exposes a function or class member function.
   * @n is a unique identifier for the callable being exposed
   * @instance is instance of an implementation.
   * @fnc is a pointer to the callable function.
   *
   * The pointer provided is used to invoke the callable when a call
   * matching the identifier is received by a service.
   *
   * In the next implementation of this, one should use unique_ptr
   * rather than a raw pointer. This will have no impact on the rest of
   * the code as it is always a reference return and not the raw pointer
   *
   * TODO(issue 21):
   */
  template <typename C, typename R, typename... Args>
  void Expose(FunctionHandlerType const &n, C *instance, R (C::*function)(Args...))
  {
    StoredType fnc(new service::CallableClassMember<C, R(Args...)>(instance, function));

    auto iter = members_.find(n);
    if (iter != members_.end())
    {
      throw serializers::SerializableException(
          error::MEMBER_EXISTS, ByteArrayType("Protocol member function already exists: "));
    }

    members_[n] = fnc;
  }

  template <typename C, typename R, typename... Args>
  void ExposeWithClientContext(FunctionHandlerType const &n, C *instance, R (C::*function)(Args...))
  {
    StoredType fnc(new service::CallableClassMember<C, R(Args...), 1>(Callable::CLIENT_CONTEXT_ARG,
                                                                      instance, function));

    auto iter = members_.find(n);
    if (iter != members_.end())
    {
      throw serializers::SerializableException(
          error::MEMBER_EXISTS, ByteArrayType("Protocol member function already exists: "));
    }

    members_[n] = fnc;
  }

  virtual void ConnectionDropped(ConnectionHandleType /*connection_handle*/)
  {}

  void DumpMemberTable()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Contents of function table");
    for (auto const &entry : members_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Entry: ", entry.first,
                     " valid: ", static_cast<bool>(entry.second));
    }
  }

private:
  static constexpr char const *LOGGING_NAME = "Protocol";

  std::map<FunctionHandlerType, StoredType> members_;
};
}  // namespace service
}  // namespace fetch
