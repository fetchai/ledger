#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

namespace fetch {
namespace kernels {
namespace basic_aritmetics {

template <typename vector_register_type>
struct Add
{
  void operator()(vector_register_type const &x, vector_register_type const &y,
                  vector_register_type &z) const
  {
    z = x + y;
  }
};

}  // namespace basic_aritmetics
}  // namespace kernels
}  // namespace fetch
