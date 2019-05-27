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

#include "vectorise/memory/shared_array.hpp"

using array_type  = fetch::memory::SharedArray<float>;
using vector_type = typename array_type::VectorRegisterType;

float InnerProduct(array_type const &A, array_type const &B)
{
  float ret = 0;

  ret = A.in_parallel().SumReduce(
      [](vector_type const &a, vector_type const &b) {
        vector_type d = a - b;
        return d * d;
      },
      B);

  return ret;
}

int main(int /*argc*/, char ** /*argv*/)
{
  array_type A, B;

  InnerProduct(A, B);

  return 0;
}
