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

//template<typename STREAM>
//class SerialisationArgumentWrapper
//{
//public:
//  using serialise_functor_type = std::function<void(STREAM &stream)>;
//
//private:
//  serialise_functor_type serialise_;
//
//public:
//  SerialisationArgumentWrapper(serialise_functor_type serialise)
//    : serialise_{ serialise }
//  {
//  }
//
//  void Serialise(STREAM &to_stream const
//  {
//    if (serialise_)
//    {
//      serialise_();
//    }
//    return stream;
//  }
//};
//
//template<typename STREAM>
//void Serialise(STREAM & stream, SerialisationArgumentWrapper<STREAM> const& ser_arg_wrapper)
//{
//  return ser_arg_wrapper.Serialise(stream)
//}

template<typename STREAM>
using lazy_argument_type = std::function<void(STREAM &stream)>;

template<typename STREAM>
void Serialise(STREAM & stream, lazy_argument_type<STREAM> const& serialisation_arg_wrapper)
{
  serialisation_arg_wrapper(stream);
}

}  // namespace serializers
}  // namespace fetch
