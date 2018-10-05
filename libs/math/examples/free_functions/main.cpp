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

//#include "math/linalg/matrix.hpp"
#include "math/meta/type_traits.hpp"
#include "math/shape_less_array.hpp"

using Type      = double;
using ArrayType = fetch::math::ShapeLessArray<Type>;

template <typename T>
fetch::math::meta::IsBlasArrayLike<T, bool> IsGreat(T const &x)
{
  return true;
}

//
// template< typename T >
// IsMathArrayLike<T, bool> IsGreat(T const& x)
//{
//  return true;
//}

int main(int argc, char **argv)
{
  //  ArrayType x;
  double x;
  std::cout << IsGreat(x) << std::endl;
}
