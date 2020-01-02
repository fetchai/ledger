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

#include <cstddef>
#include <iostream>
#include <vector>

using array_type = std::vector<double>;

double InnerProduct(array_type const &A, array_type const &B)
{
  double ret = 0.0;

  for (std::size_t i = 0; i < A.size(); ++i)
  {
    double d = A[i] - B[i];
    ret += d * d;
  }

  return ret;
}

int main(int /*argc*/, char ** /*argv*/)
{
  std::vector<double> A, B;

  double product = InnerProduct(A, B);
  std::cout << "product = " << product << std::endl;

  return 0;
}
