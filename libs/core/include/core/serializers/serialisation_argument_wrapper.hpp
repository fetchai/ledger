#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <functional>

namespace fetch {
namespace serializers {

template<typename T>
class LazyEvalArgument : public std::reference_wrapper<T const>
{
public:
  using base_type = std::reference_wrapper<T const>;
  using base_type::base_type;
  using base_type::operator=;
  using base_type::operator();
};

template<typename T>
LazyEvalArgument<T> LazyEvalArgumentFactory(T const& lamda)
{
  return LazyEvalArgument<T>{lamda};
}

template<typename STREAM, typename T>
void Serialize(STREAM & stream, LazyEvalArgument<T> const& lazyEvalArgument)
{
  lazyEvalArgument(stream);
}

}  // namespace serializers
}  // namespace fetch
