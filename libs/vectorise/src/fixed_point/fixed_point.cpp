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

#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace fixed_point {

/* e = 2.718281828459045235360287471352662498 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_E(2, 0xB7E151628AED2A6A);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_E{FixedPoint<64, 64>::CONST_E};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_E(FixedPoint<64, 64>::CONST_E);

/* log_2(e) = 1.442695040888963407359924681001892137 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LOG2E(1, 0x71547652B82FE177);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LOG2E{FixedPoint<64, 64>::CONST_LOG2E};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LOG2E{FixedPoint<64, 64>::CONST_LOG2E};

/* log_2(10) = 3.32192809488736234787 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LOG210(3, 0x5269E12F346E2BF9);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LOG210{FixedPoint<64, 64>::CONST_LOG210};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LOG210{FixedPoint<64, 64>::CONST_LOG210};

/* log_10(e) = 0.434294481903251827651128918916605082 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LOG10E(0, 0x6F2DEC549B9438CA);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LOG10E{FixedPoint<64, 64>::CONST_LOG10E};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LOG10E{FixedPoint<64, 64>::CONST_LOG10E};

/* ln(2) = 0.693147180559945309417232121458176568 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LN2(0, 0xB17217F7D1CF79AB);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LN2{FixedPoint<64, 64>::CONST_LN2};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LN2{FixedPoint<64, 64>::CONST_LN2};

/* ln(10) = 2.302585092994045684017991454684364208 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_LN10(2, 0x4D763776AAA2B05B);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_LN10{FixedPoint<64, 64>::CONST_LN10};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_LN10{FixedPoint<64, 64>::CONST_LN10};

/* Pi = 3.141592653589793238462643383279502884 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_PI(3, 0x243F6A8885A308D3);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_PI{FixedPoint<64, 64>::CONST_PI};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_PI{FixedPoint<64, 64>::CONST_PI};

/* Pi/2 = 1.570796326794896619231321691639751442 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_PI_2(1, 0x921FB54442D18469);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_PI_2{FixedPoint<64, 64>::CONST_PI_2};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_PI_2{FixedPoint<64, 64>::CONST_PI_2};

/* Pi/4 = 0.785398163397448309615660845819875721 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_PI_4(0, 0xC90FDAA22168C234);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_PI_4{FixedPoint<64, 64>::CONST_PI_4};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_PI_4{FixedPoint<64, 64>::CONST_PI_4};

/* 1/Pi = 0.318309886183790671537767526745028724 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_INV_PI(0, 0x517CC1B727220A94);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_INV_PI{FixedPoint<64, 64>::CONST_INV_PI};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_INV_PI{FixedPoint<64, 64>::CONST_INV_PI};

/* 2/Pi = 0.636619772367581343075535053490057448 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_TWO_INV_PI(0, 0xA2F9836E4E441529);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_TWO_INV_PI{FixedPoint<64, 64>::CONST_TWO_INV_PI};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_TWO_INV_PI{FixedPoint<64, 64>::CONST_TWO_INV_PI};

/* 2/Sqrt(Pi) = 1.128379167095512573896158903121545172 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_TWO_INV_SQRTPI(1, 0x20DD750429B6D11A);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_TWO_INV_SQRTPI{
    FixedPoint<64, 64>::CONST_TWO_INV_SQRTPI};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_TWO_INV_SQRTPI{
    FixedPoint<64, 64>::CONST_TWO_INV_SQRTPI};

/* Sqrt(2) = 1.414213562373095048801688724209698079 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_SQRT2(1, 0x6A09E667F3BCC908);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_SQRT2{FixedPoint<64, 64>::CONST_SQRT2};
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_SQRT2{FixedPoint<64, 64>::CONST_SQRT2};

/* 1/Sqrt(2) = 0.707106781186547524400844362104849039 */
template <>
FixedPoint<64, 64> const FixedPoint<64, 64>::CONST_INV_SQRT2(0, 0xB504F333F9DE6484);
template <>
FixedPoint<32, 32> const FixedPoint<32, 32>::CONST_INV_SQRT2(0, 0xB504F333);
template <>
FixedPoint<16, 16> const FixedPoint<16, 16>::CONST_INV_SQRT2(0, 0xB504);

}  // namespace fixed_point
}  // namespace fetch
