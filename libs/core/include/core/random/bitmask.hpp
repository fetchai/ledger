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
namespace random {

template <typename W, uint8_t B = 12, bool MSBF = true>
class BitMask
{
public:
  using float_type = double;
  using word_type  = W;
  enum
  {
    BITS_OF_PRECISION = B
  };

  void SetProbability(float_type const &d)
  {
    word_type w = word_type(d * word_type(-1));
    if (d < 0) w = 0;
    if (d >= 1) w = word_type(-1);

    if (MSBF)
    {
      w ^= (w >> 1);
      w >>= 8 * sizeof(word_type) - BITS_OF_PRECISION;
      for (std::size_t i = BITS_OF_PRECISION - 1; i < BITS_OF_PRECISION; --i)
      {
        mask_[i] = -(w & 1);
        w >>= 1;
      }
    }
    else
    {
      w ^= (w << 1);
      for (std::size_t i = 0; i < BITS_OF_PRECISION; ++i)
      {
        mask_[i] = -(w & 1);
        w >>= 1;
      }
    }
  }

  word_type const &operator[](std::size_t const &n) const { return mask_[n]; }

private:
  word_type mask_[BITS_OF_PRECISION] = {0};
};
}  // namespace random
}  // namespace fetch
