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

#include <cassert>
#include <iostream>
#include <math/linalg/matrix.hpp>

namespace fetch {
namespace math {
namespace combinatorics {

std::size_t factorial(std::size_t n)
{
  /** Calculates the factorial of an integer (n!)
 * @param n: Integer for which this function calculates the factorial
 * @return: Integer solution to n!
 */
  // Trivial case
  if (n == 0){
    return 1;
  }

  // General solution
  for(std::size_t i = n-1; i > 1; i--)
    n *= i;

  return n;
}



// matrix
// type
fetch::math::linalg::Matrix<double> combinations(std::size_t n, std::size_t r)
{
  /** Calculates all size r combinations of n items
   * @param n: The total number of items to choose from, where items will be selected from [1,...,n]
   * @param r: The number of items to select
   * @return: Matrix of size (num possible combinations, r), where each row contains a unique combination of r items
   */
  // assert (n != 1);
  assert (r <= n);
  if (r == 0){
    fetch::math::linalg::Matrix<double> output_array{};
    return output_array;
  }

  std::size_t n_combinations = factorial(n)/factorial(r)/factorial(n-r);
  std::size_t current_dim = 0;
  std::size_t current_row = 0;

  std::vector<bool> v(static_cast<unsigned long>(n));
  std::fill(v.end() - static_cast<int>(r), v.end(), true);

  fetch::math::linalg::Matrix<double> output_array{n_combinations, r};
  do {
    for (std::size_t i = 0; i < n; ++i) {
      if (v[static_cast<unsigned long>(i)]) {
        std::size_t dim = (i+1);
        output_array.Set(current_row, current_dim, static_cast<float>(dim));
        if (current_dim == r-1){
          current_row += 1;
        }
        current_dim = current_dim + 1;
        current_dim = current_dim % r;

      }
    }
  }while (std::next_permutation(v.begin(), v.end()));
  return output_array;
}


} // namespace fetch
} // namespace math
} // namespace combinatorics