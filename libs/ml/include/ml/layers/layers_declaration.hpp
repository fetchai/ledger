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

#include "ml/graph_declaration.hpp"
#include "ml/subgraph_declaration.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class Convolution1D;

template <class T>
class Convolution2D;

template <class T>
class FullyConnected;

template <class T>
class PRelu;

template <class T>
class SelfAttention;

template <class T>
class SkipGram;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
