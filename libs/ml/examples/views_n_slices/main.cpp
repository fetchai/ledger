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

#include "math/tensor.hpp"

/*using DataType  = double;
using ArrayType = Tensor<DataType>;
using SizeType  = typename ArrayType::SizeType;*/

int main(int ac, char **av)
{
  (void)ac;
  (void)av;

  using DataType  = double;
  using ArrayType = fetch::math::Tensor<DataType>;
  using SizeType  = typename ArrayType::SizeType;

  SizeType xs = 3;
  SizeType ys = 3;
  SizeType zs = 3;

  ArrayType t({xs, ys, zs});

  for (SizeType i = 0; i < xs; i++)
  {
    for (SizeType j = 0; j < ys; j++)
    {
      for (SizeType k = 0; k < zs; k++)
      {
        t(i, j, k) = static_cast<DataType>(i * 100 + j * 10 + k);
      }
    }
  }

  auto vt = t.Slice(0, 0).Slice(1, 1).Slice(2, 2);

  auto it2 = vt.begin();
  while (it2.is_valid())
  {
    std::cout << *it2 << std::endl;
    ++it2;
  }

  return 0;
}
