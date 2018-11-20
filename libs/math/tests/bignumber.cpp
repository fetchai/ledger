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

#include "math/bignumber.hpp"
#include "byte_array/encoders.hpp"
#include <iomanip>

#include "unittest.hpp"
using namespace fetch::math;
using namespace fetch::byte_array;

int main()
{
  SCENARIO("testing shifting")
  {
    BigUnsigned n1(0);
    std::cout << "Original: " << ToHex(n1) << std::endl;
    SECTION("testing elementary left shifting")
    {
      n1 = 3;
      EXPECT(n1[0] == 3);
      n1 <<= 8;
      EXPECT(n1[0] == 0);
      EXPECT(n1[1] == 3);
      n1 <<= 7;
      EXPECT(n1[0] == 0);
      EXPECT(int(n1[1]) == 128);
      EXPECT(int(n1[2]) == 1);
    };

    SECTION("testing incrementer for a million increments")
    {
      for (std::size_t count = 0; count < (1 << 12); ++count)
      {
        union
        {
          uint32_t value;
          uint8_t  bytes[sizeof(uint32_t)];
        } c;

        for (std::size_t i = 0; i < sizeof(uint32_t); ++i)
        {
          c.bytes[i] = n1[i + 1];
        }
        SILENT_EXPECT(c.value == count);

        for (std::size_t i = 0; i < (1ul << 8); ++i)
        {
          DETAILED_EXPECT(i == int(n1[0]))
          {
            CAPTURE(i);
            CAPTURE(count);
          };
          ++n1;
        }
      }
    };

    SECTION("testing comparisons")
    {
      BigUnsigned a(0), b(0);

      for (std::size_t count = 0; count < (1 << 8); ++count)
      {
        SILENT_EXPECT(a == b);

        for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
        {
          ++a;
          SILENT_EXPECT(b < a);
        }

        for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
        {
          SILENT_EXPECT(b < a);
          ++b;
        }

        SILENT_EXPECT(a == b);

        for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
        {
          ++b;
          SILENT_EXPECT(b > a);
        }

        for (std::size_t i = 0; i < (1ul << 8) / 2; ++i)
        {
          SILENT_EXPECT(b > a);
          ++a;
        }
      }
    };
  };

  return 0;
}
