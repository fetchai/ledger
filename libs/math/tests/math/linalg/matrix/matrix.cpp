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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include "math/free_functions/free_functions.hpp"
#include <math/linalg/matrix.hpp>

using namespace fetch::math::linalg;

using data_type            = float;
using container_type       = fetch::memory::SharedArray<data_type>;
using matrix_type          = Matrix<data_type, container_type>;
using vector_register_type = typename matrix_type::vector_register_type;

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _M = Matrix<D, _S<D>>;

TEST(matrix, basic_info)
{
  std::cout << "Vector SIMD block count for float: "
            << _M<float>::vector_register_type::E_BLOCK_COUNT << std::endl;
  std::cout << "Vector SIMD block count for double: "
            << _M<double>::vector_register_type::E_BLOCK_COUNT << std::endl;

  std::cout << "Vector SIMD vector size for float: "
            << _M<float>::vector_register_type::E_VECTOR_SIZE << std::endl;
  std::cout << "Vector SIMD vector size for double: "
            << _M<double>::vector_register_type::E_VECTOR_SIZE << std::endl;
}

TEST(matrix, approx_softmax)
{
  _M<double> A, R;

  R.Resize(7, 7);
  A = _M<double>(R"(
0.649920943286 -0.4335249319 -1.24190820228 -0.159719002188 -0.750455901033 0.17660743611 -1.6538074988 ;
-1.45410231783 -0.0214562678207 -0.265842280326 0.310584468792 0.047163893089 0.442820425979 -0.308607040063 ;
-1.87302591356 0.355462462953 1.85310670937 -0.463362692725 -0.820154428828 -1.23581665316 -0.0808778286129 ;
-0.34374468999 1.54865554346 -1.07669460669 -0.612113018273 0.37759895184 1.95708030904 -0.39417363816 ;
-1.7835679676 0.134560286028 0.693684324184 -1.41934800163 0.0772682670942 1.35825798964 0.197195222647 ;
-0.830042692577 0.248720257196 -0.232392377288 -0.338130303769 -1.81619181952 -0.519176393062 -1.60435541774 ;
0.0278604896043 -0.434628108296 -0.385180745714 0.252128451234 -0.405710699869 -0.761730146467 -0.903675404106
)");
  R.ApproxSoftMax(A);
}

// TEST(matrix, int_addition)
//{
//  _M<int> A, B, C, R;
//
//  R.Resize(7, 7);
//  A = _M<int>(R"(
//      0 4 4 2 -9 -7 7 ;
//      2 -2 4 5 -1 3 0 ;
//      -3 9 8 0 -10 -2 -5 ;
//      8 -4 4 -5 4 -2 -9 ;
//      9 1 3 -1 3 -5 -5 ;
//      -3 8 -10 5 8 9 0 ;
//      -6 -4 3 -3 -5 2 2
//      )");
//  B = _M<int>(R"(
//      -4 -5 5 -1 -10 -9 9 ;
//      -10 -3 -6 7 -9 -7 -6 ;
//      1 7 3 0 0 9 9 ;
//      8 -6 5 -3 3 2 9 ;
//      -2 -7 5 2 4 -7 -5 ;
//      -4 -5 2 -2 9 -9 -9 ;
//      2 7 4 6 -3 -4 5
//      )");
//  C = _M<int>(R"(
//      -4 -1 9 1 -19 -16 16 ;
//      -8 -5 -2 12 -10 -4 -6 ;
//      -2 16 11 0 -10 7 4 ;
//      16 -10 9 -8 7 0 0 ;
//      7 -6 8 1 7 -12 -10 ;
//      -7 3 -8 3 17 0 -9 ;
//      -4 3 7 3 -8 -2 7
//      )");
//
//  ASSERT_TRUE(fetch::math::Add(A, B).AllClose(C));
////  ASSERT_TRUE((R = A, R.InlineAdd(B)).AllClose(C));
////  ASSERT_TRUE((R = A, (R += B)).AllClose(C));
////  ASSERT_TRUE((Add(A,B)).AllClose(C));
//}

//
// TEST(matrix, int_multiplication){
//  _M<int> A, B, C, R;
//
//  R.Resize(7, 7);
//  A = _M<int>(R"(
//      -9 -7 -5 -2 -5 -4 -10 ;
//      -8 -8 9 2 -4 9 -9 ;
//      -2 4 -2 1 -6 7 3 ;
//      5 -2 -9 -2 -1 -1 3 ;
//      -7 -5 4 -2 -8 -2 -1 ;
//      8 0 -1 -8 -2 -2 -2 ;
//      -7 -9 -6 2 5 -3 -7
//      )");
//  B = _M<int>(R"(
//      5 -3 -7 -1 3 -6 -5 ;
//      1 -10 0 -4 8 -8 9 ;
//      -4 -8 1 -10 2 -4 -6 ;
//      3 -8 6 -10 -9 8 -6 ;
//      -4 -10 6 4 -6 -6 -8 ;
//      1 -7 0 -6 -10 -9 -10 ;
//      1 3 -3 0 -1 1 -5
//      )");
//  C = _M<int>(R"(
//      -45 21 35 2 -15 24 50 ;
//      -8 80 0 -8 -32 -72 -81 ;
//      8 -32 -2 -10 -12 -28 -18 ;
//      15 16 -54 20 9 -8 -18 ;
//      28 50 24 -8 48 12 8 ;
//      8 0 0 48 20 18 20 ;
//      -7 -27 18 0 -5 -3 35
//      )");
//
//  ASSERT_TRUE(fetch::math::Multiply(A, B).AllClose(C));
//  ASSERT_TRUE((R = A, R.InlineMultiply(B)).AllClose(C));
//  ASSERT_TRUE((R = A, R *= B).AllClose(C));
//  ASSERT_TRUE((A * B).AllClose(C));
//
//};
//
// TEST(matrix, int_subtraction){
//  _M<int> A, B, C, R;
//
//  R.Resize(7, 7);
//  A = _M<int>(R"(
//      -4 7 6 -6 -7 -5 0 ;
//      -2 -10 -5 6 8 -8 4 ;
//      8 -10 1 9 1 0 -6 ;
//      -7 -10 -6 -10 8 -4 2 ;
//      -9 7 -2 -8 3 3 -7 ;
//      4 8 -5 -7 -4 -4 2 ;
//      -4 6 -8 0 1 8 5
//      )");
//  B = _M<int>(R"(
//      9 2 7 -7 -6 -10 9 ;
//      3 2 -5 1 6 2 -3 ;
//      -7 -3 2 -6 -5 2 -6 ;
//      0 3 0 -1 -7 -1 -1 ;
//      -10 9 -1 3 0 -3 8 ;
//      -9 -10 -1 -4 6 0 5 ;
//      -5 -4 -1 0 6 2 -9
//      )");
//  C = _M<int>(R"(
//      -13 5 -1 1 -1 5 -9 ;
//      -5 -12 0 5 2 -10 7 ;
//      15 -7 -1 15 6 -2 0 ;
//      -7 -13 -6 -9 15 -3 3 ;
//      1 -2 -1 -11 3 6 -15 ;
//      13 18 -4 -3 -10 -4 -3 ;
//      1 10 -7 0 -5 6 14
//      )");
//
//  ASSERT_TRUE(R.Subtract(A, B).AllClose(C));
//  ASSERT_TRUE((R = A, R.InlineSubtract(B)).AllClose(C));
//  ASSERT_TRUE((R = B, R.InlineReverseSubtract(A)).AllClose(C));
//  ASSERT_TRUE((R = A, R -= B).AllClose(C));
//  ASSERT_TRUE((A - B).AllClose(C));
//};
//
// TEST(matrix, matrix_int_dot){
//
//  _M<int> A, B, C, R;
//
//  R.Resize(7, 7);
//  A = _M<int>(R"(
//      -6 -2 -3 -9 -6 8 -5 ;
//      -7 -5 -1 -7 -9 9 6 ;
//      0 2 7 5 6 -1 -4 ;
//      -8 -2 -8 0 5 -2 -1 ;
//      -7 -2 -6 -5 8 2 -6 ;
//      5 9 9 8 -1 -8 0 ;
//      8 3 0 8 -1 -3 4
//      )");
//  B = _M<int>(R"(
//      -4 -2 1 2 -3 -5 -4 ;
//      8 1 5 7 9 -9 -7 ;
//      3 -7 -1 2 -5 -4 7 ;
//      8 8 4 3 1 -4 -10 ;
//      9 3 7 6 8 -4 -8 ;
//      8 8 9 7 -7 -4 6 ;
//      -7 -4 -3 9 -5 -1 9
//      )");
//  C = _M<int>(R"(
//      -28 25 -4 -84 -73 93 158 ;
//      -122 -19 -59 -9 -191 106 306 ;
//      151 19 68 36 63 -82 -105 ;
//      28 73 10 -39 105 79 -71 ;
//      84 78 61 -47 108 63 -56 ;
//      70 -67 -6 53 77 -138 -140 ;
//      -5 8 9 70 4 -87 -107
//      )");
//
//  ASSERT_TRUE(R.Dot(A, B).AllClose(C));
//// ASSERT_TRUE( ( R = A, R.InlineDot(B) ).AllClose(C) );
//
//};
//
TEST(matrix, float_addition)
{
  _M<float> A, B, C, R;

  R.Resize(7, 7);
  A = _M<float>(R"(
-0.26227950736 -1.36910925273 0.980054371644 1.32713840772 2.31283768424 2.53244524973
-1.88827862008 ; -0.303353383517 -0.542714130844 -0.377449354595 1.47318869719 1.61588229672
 0.374438688403 0.0845604022731 ; 0.419208029381 1.32654055661 0.541074147177 0.721043889254
 -0.416337124677 -1.83878791782 -1.65745783609 ; -0.0196045660289 0.371971795502 0.61087551298
-2.00689262018 -0.0901101382207 -0.738254737558 -0.103902836988 ; 1.20446095598 -0.985411627634
 -1.27691759531 1.04943611976 -1.04116477047 -0.0164841701504 0.460749899135 ; -0.231233553106
 0.014941806308 -1.1296995159 0.722239971199 -1.07106187847 0.965245439425 -1.86864275864 ;
-0.990668403946 -0.691268306401 -0.0152358049645 -0.00899989245372 -0.322276526537 -0.239793448279
 0.122430357946
)");
  B = _M<float>(R"(
-0.0887649625032 0.366927692567 -0.557966305341 -0.492416501045 0.354352257055
-0.603233812291 1.27802260565 ; -0.177136514743 -0.085377708792 -2.40770923588 -1.30250416986
 0.464035768561 0.0544515541586 -0.510097280579 ; 0.832898956022 2.12291218258 -0.798804696892
 -0.152395308973 -1.07735930124 2.09989096863 -1.51552522947 ; 0.127896540686 -0.871316493564
 0.412012717363 -0.135635656155 1.22712612926 0.190968745042 0.0148465799957 ;
-0.0652351318294 2.04240756572 -0.0406457798846 -0.881670455861 -0.184864158644 -0.315222131885
-0.58578875395 ; -1.08360388482 1.58710471748 0.176426043112 1.49521249733 -1.80647049259
-0.973552721569 -0.891016552951 ; -0.555054723934 -1.30205618298 -0.854459008934 1.43676094037
 0.460165328107 -0.688728346225 1.89810801774
)");
  C = _M<float>(R"(
-0.351044469863 -1.00218156017 0.422088066303 0.83472190668 2.66718994129 1.92921143744
-0.610256014439 ; -0.48048989826 -0.628091839636 -2.78515859047 0.170684527329 2.07991806528
 0.428890242561 -0.425536878306 ; 1.2521069854 3.44945273919 -0.257730549715 0.568648580281
 -1.49369642592 0.26110305081 -3.17298306556 ; 0.108291974657 -0.499344698062 1.02288823034
 -2.14252827633 1.13701599104 -0.547285992515 -0.0890562569925 ; 1.13922582415 1.05699593808
 -1.3175633752 0.167765663902 -1.22602892912 -0.331706302036 -0.125038854815 ;
-1.31483743793 1.60204652379 -0.953273472793 2.21745246853 -2.87753237106 -0.00830728214452
-2.75965931159 ; -1.54572312788 -1.99332448938 -0.869694813898 1.42776104792 0.13788880157
-0.928521794504 2.02053837568
)");
  Add(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R = A, R.InlineAdd(B)).AllClose(C));
};

TEST(matrix, float_multiplication)
{
  _M<float> A, B, C, R;

  R.Resize(7, 7);
  A = _M<float>(R"(
-2.21533136471 0.0968145260677 -0.0237083616488 -0.176699137663 0.383612322767 -0.0172887091865
-0.846230033918 ; -0.292288573517 -1.55991662607 -0.844978624433 -0.0139608253681 0.0952790327505
-0.101895369228 1.27567548539 ; -0.432843985554 1.50113824379 -0.00216896382732
-1.0907925226 1.16732231635 -0.394374777224 -1.48340035033 ; 0.809822197143 -3.30167772353
 0.582972021274 1.2877145681 -0.237919383561 -0.83521201827 -0.99841515053 ; 0.669010916895
 0.91741252505 -0.0331500109933 1.05519567062 -1.44825823605 -2.12552255926 0.506912678122 ;
-0.465823913019 -0.905765315065 -0.0299858337226 0.0309648505483 -0.39228714482 -0.186962756794
 0.0683186423863 ; -1.08806370716 0.47923559741 0.364946653677 0.795131463691
 0.849256309133 1.09063376415 -0.180109382732
)");
  B = _M<float>(R"(
 0.247953119919 0.928840460892 0.652521990712 -0.196164734243 -1.6003396297 1.21994740296
 0.559105610895 ; -0.824445861646 0.0800512160619 1.38041270396 -1.87437885298
 2.50025561541 2.40190228313 0.0914466705149 ; 0.519215278722 -1.25906364386 -1.6334465774
 -0.242087380545 -1.97215159738 -0.477620729128 0.474661846434 ; 0.963725406191
 0.589219808908 1.03267443926 -0.423135887991 -0.0549458629842 0.495846640641 -0.216277975422 ;
 1.50322936381 -0.592852166679 0.762841459844 -1.22311307657 0.754713202448 1.35720786854
 -1.00013940192 ; -0.379978335226 0.900681906142 -2.49813895144 1.42890205707 -0.00756639461945
-0.822424990768 -1.38179927135 ; -0.108225892327 -1.55964619032 -0.98987497676 -0.216863390207
 0.775860601433 -0.878154212202 -1.02333097332
)");
  C = _M<float>(R"(
-0.549298323535 0.0899252490138 -0.0154702273396 0.0346621393807 -0.613910002566 -0.0210913158725
-0.473131960071 ; 0.240976104842 -0.124873222872 -1.16641922775 0.0261678758402 0.238221936665
 -0.24474271999 0.116656275797 ; -0.224739210602 -1.89002858716 0.00354288654025 0.264067104513
-2.30213657085 0.188361568647 -0.704113549289 ; 0.780446225884 -1.94541391733 0.602020305172
 -0.54487824725 0.0130726858504 -0.414137073483 0.215935207388 ; 1.00567685499 -0.543890003215
 -0.02528820278 -1.29062362308 -1.0930196113 -2.88477594219 -0.50698334272 ; 0.177002994977
 -0.81580643049 0.0749087792137 0.0442457386455 0.00296819934184 0.15376284353 -0.0944026502689 ;
 0.117756665616 -0.747437973767 -0.361251560327 -0.172434904876 0.658904510775 -0.957744633955
 0.184311509935
)");
  Multiply(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R = A, R.InlineMultiply(B)).AllClose(C));
};

TEST(matrix, float_subtraction)
{
  _M<float> A, B, C, R;

  R.Resize(7, 7);
  A = _M<float>(R"(
 1.51444797718 -0.98640415793 -0.0200743705825 0.831701879253 1.20912727183 0.664420835443
 -0.452703635763 ; 1.41856575968 -1.31187263188 -0.958044650641 -1.54413513577 0.497343638137
 -0.358371208138 0.394057744427 ; -0.833643230613 -0.222994844315 0.357188851611 -0.146294658399
-1.91117431268 -0.391228344281 0.479519621373 ; 0.495534428331 -0.11391893858
 0.827036694524 1.15928763086 0.585258591524 -0.342305272749 0.399957359112 ;
-0.293025618074 1.53727358723 -0.418703136494 0.527599763144 -1.1621352032 -0.113047234745
-1.02614306854 ; -1.11514677665 -1.84715600051 0.745137670981 0.0249095876836
-2.32179845436 1.21300429596 1.73725977249 ; -0.392804961513 1.70973797507 -0.881505719826
-0.898411674393 1.3972147298 2.49759910381 -0.305206226544
)");
  B = _M<float>(R"(
 0.116896812876 -0.0578410401649 -0.49806763035 -0.571651733328 -0.843885137366 2.21321406155
 -1.8243594932 ; -0.551910268507 0.255589913474 -0.254019168301 -0.0132001952087 0.0155049650473
 0.643479985293 0.320458125126 ; -0.663684762302 -0.611148755406 0.63078447599 0.0689489694442
-0.797675925205 -1.08734001996 -1.77849007709 ; -0.214298899257 0.468611720009 1.64134807784
 0.950346008363 1.38656653757 0.968344937148 0.36812065221 ; 0.677560571479 2.21754214015
 0.817085406964 -0.489602424579 0.268576920892 -2.94607762036 0.0735026652726 ; 0.564854182634
 0.252768045622 1.60970642168 0.518967111035 0.75196912579 -1.12847853762 -0.145559584046 ;
 0.238132871817 0.53623509709 0.958159356866 -1.13155141951 0.146206956745 0.344291069376
 -1.47303107777
)");
  C = _M<float>(R"(
 1.3975511643 -0.928563117765 0.477993259767 1.40335361258 2.05301240919
 -1.5487932261 1.37165585744 ; 1.97047602819 -1.56746254535 -0.70402548234 -1.53093494056
 0.48183867309 -1.00185119343 0.073599619301 ; -0.169958468311 0.38815391109 -0.273595624379
-0.215243627844 -1.11349838748 0.696111675676 2.25800969846 ; 0.709833327588 -0.582530658588
 -0.814311383316 0.208941622497 -0.801307946046 -1.3106502099 0.0318367069014 ; -0.970586189554
-0.680268552924 -1.23578854346 1.01720218772 -1.43071212409 2.83303038562 -1.09964573381 ;
-1.68000095929 -2.09992404614 -0.864568750696 -0.494057523352 -3.07376758015
 2.34148283358 1.88281935654 ; -0.63093783333 1.17350287798 -1.83966507669
 0.23313974512 1.25100777305 2.15330803444 1.16782485123
)");
  Subtract(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R.Copy(A), R.InlineSubtract(B)).AllClose(C));
  ASSERT_TRUE((R.Copy(B), R.InlineReverseSubtract(A)).AllClose(C));
};

TEST(matrix, dot_float)
{
  _M<float> A, B, C, R;

  R.Resize(7, 7);
  A = _M<float>(R"(
-0.426029076554 1.6718736486 -0.730266938927 1.29031075872 -0.265882310105 0.822123890687
-1.08064364867 ; 1.12931564409 -0.327294069507 1.84809881578 -0.993033681697 2.01283403016
 0.346637246942 0.421596268546 ; -1.0033535669 0.614678815098 -0.961138373929 0.15698936597
-0.566988257848 -1.22897594376 -0.117787966115 ; 0.360685100396 -1.00319955825 1.24186641023
 -1.26817311457 0.0157827692772 -0.612702232413 0.130069406443 ; -0.0559251654915 -1.46576484448
-0.85858308295 -1.1118723633 -0.232755812504 0.38566303467 -0.190691239387 ; -1.39859270469
-2.12182728163 -0.678579596248 1.59822542265 -1.29537352868 2.17688373984 -0.531810828573 ;
 0.0522484247056 0.0257453964073 1.72473120153 0.368529339869 -1.51081293676 0.0373595975009
 -0.745989259716
)");
  B = _M<float>(R"(
 1.38155954016 -0.453115279902 0.446140794892 0.659431562102 -0.356511502103 -0.927187551434
 -0.203811630902 ; 1.55592655083 0.167909059458 0.205908284446 0.236707657789
 -0.129767784608 1.42754909743 -0.570757653841 ; -0.685849230041 1.54333903014 -0.0514279379829
 0.028778003398 0.364863472473 0.547487377784 -1.24979467183 ; 2.23303971351 0.793847891434
 0.210185418447 -0.0954888607208 -0.889858539879 -0.0529901024358 0.498280891292 ; -1.19343879225
 0.24684336227 -1.80587610637 -0.226569918238 0.0174641758551 -0.385714816615 0.199907197369 ;
 0.84936633544 -0.522874627452 1.35489905642 2.0567050316 1.22996769314 -1.45750207982
 0.668084158201 ; -1.39701042321 0.105319162773 0.358307904316 0.718408779724 0.314656329194
 0.30967567232 0.207755615965
)");
  C = _M<float>(R"(
 7.92016530952 -0.238287468813 1.66978654713 0.945345738124 -0.813201806195 0.883163361154
 0.95980229703 ; -5.13077611599 1.85726859044 -2.88153301932 1.37500300472 1.79198449985
-1.60093033858 -2.12636397346 ; 0.377327903039 -0.310653993848 -0.922075415745 -3.04259263947
 -1.77100645158 3.24670581238 0.174237404437 ; -5.46716367223 0.915967247987 -1.18811539413
-1.01306369832 0.870799807741 -0.0922260902744 -2.06407136116 ; -3.38013631869 -2.7077080477
 0.358228641121 0.406562844638 1.29657651331 -2.98312308116 1.53853113893 ; 2.93852369086
 -1.01508229538 4.40814035799 2.79197301823 1.59171216027 -5.02631545926 4.22544933601 ;
 2.62922686278 2.46401404995 2.52903708718 -0.0617905316698 0.0642201006278 1.21032540211
 -2.42931843962
)");

  //    ASSERT_TRUE(R.Dot(A, B).AllClose(C));
  // ASSERT_TRUE( ( R.Copy(A), R.InlineDot(B) ).AllClose(C) );
};

TEST(matrix, float_division)
{
  _M<float> A, B, C, R;

  R.Resize(7, 7);
  A = _M<float>(R"(
 2.28695181593 0.485244582845 0.354977016066 -0.153741790569 1.30849306789 -0.531606158678
 -0.270294238066 ; -0.942017039362 0.709512225463 -1.44039226674 1.26182363815 -0.0329056404826
 0.803457777133 -1.00093932784 ; -0.160176058594 0.48612085911 -0.0971798465026 -0.586002344131
 0.129700682274 -0.299319934068 0.0757826132841 ; -0.363930240769 0.547149219891 0.39876280666
-1.11688197552 0.403361657453 -0.750956200908 0.69351288904 ; -1.06310646307 0.976908629122
 0.874248745501 -1.38719234294 -1.87815473775 1.21472971542 -1.28118799946 ; 0.686858008807
 -0.413227536148 0.165773581812 1.18788615257 1.51358569734 0.502597935783 -0.472827766292 ;
-0.324107812816 -0.59836251229 -0.851394911036 1.16206520917 0.383594122257 -0.873442958014
-2.31616736995
)");
  B = _M<float>(R"(
 1.64104892817 0.820226845431 0.712438940732 1.28826226015 -0.250977057623
 -1.18204789907 1.5941545516 ; -0.598770180125 -0.385413822547 0.370698660152 -2.42022357988
-1.10107052522 -0.628957797218 0.304443898685 ; -0.720123295908 1.04467884226
 1.3636255562 2.96129390622 -1.01563433661 -0.157725165131 -1.0633746992 ; -1.2521950538
 0.801347594676 0.467181751912 0.115882077847 -2.74540405117 0.888415742904 -1.01192870669 ;
-1.34405753427 0.661393434165 1.34505813548 -0.128461858134 -1.11194472396 -0.194353507088
-0.285438686358 ; -0.153116041857 -0.417920260937 0.116897575331 -0.535249874786
 0.269946263895 1.01170365953 -0.223320009101 ; 1.07426801559 -1.11250267042
 1.75053138831 1.13168578971 -0.911777670379 1.29212351153 -0.502595370565
)");
  C = _M<float>(R"(
 1.39359148693 0.591598026264 0.49825605504 -0.119340444353 -5.2135963354 0.449733178406
 -0.169553345876 ; 1.57325309548 -1.84091016968 -3.88561497941 -0.521366558295 0.0298851342661
 -1.27744306643 -3.28776281002 ; 0.222428658403 0.465330434049 -0.071265785582 -0.197887262355
 -0.127704113182 1.89773099188 -0.0712661429135 ; 0.290633827107 0.682786375758 0.853549619667
 -9.63809068901 -0.146922511199 -0.845275657153 -0.685337696672 ; 0.790967972693 1.47704615537
 0.649970973328 10.7984763967 1.68907203504 -6.25010442888 4.48848758313 ; -4.48586575565
 0.988771243638 1.41810966859 -2.21931140674 5.60698887067 0.496783747938 2.11726556968 ;
-0.301701072834 0.537852652581 -0.486363693173 1.02684439421 -0.420710151958
-0.675974819917 4.60841365759
)");
  Divide(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R.Copy(A), R.InlineDivide(B)).AllClose(C));
  ASSERT_TRUE((R.Copy(B), R.InlineReverseDivide(A)).AllClose(C));
};

TEST(matrix, double_addition)
{
  _M<double> A, B, C, R;

  R.Resize(7, 7);
  A = _M<double>(R"(
 0.649920943286 -0.4335249319 -1.24190820228 -0.159719002188 -0.750455901033 0.17660743611
 -1.6538074988 ; -1.45410231783 -0.0214562678207 -0.265842280326 0.310584468792 0.047163893089
 0.442820425979 -0.308607040063 ; -1.87302591356 0.355462462953 1.85310670937 -0.463362692725
-0.820154428828 -1.23581665316 -0.0808778286129 ; -0.34374468999 1.54865554346 -1.07669460669
-0.612113018273 0.37759895184 1.95708030904 -0.39417363816 ; -1.7835679676 0.134560286028
 0.693684324184 -1.41934800163 0.0772682670942 1.35825798964 0.197195222647 ; -0.830042692577
 0.248720257196 -0.232392377288 -0.338130303769 -1.81619181952 -0.519176393062 -1.60435541774 ;
 0.0278604896043 -0.434628108296 -0.385180745714 0.252128451234 -0.405710699869 -0.761730146467
 -0.903675404106
)");
  B = _M<double>(R"(
-2.42640999063 -0.904682117192 0.705118384819 -0.563105402223 -0.103587138073
 0.758118499738 1.12152282318 ; -0.580954759848 0.436126745818 0.0549494645983 -0.337610267879
-0.797746508054 -0.811034445559 -1.61024410156 ; 0.537618270678 -0.806289353304 0.666921105315
 -0.525986694329 0.787589813607 -0.498059836321 -0.164659996246 ; -0.299554201481 -0.823012596979
-0.810643213431 -0.806962864256 -1.35875644287 0.289068858167 1.38013368071 ;
 0.223381451967 1.5357020432 0.887500617767 -0.259587717138 -0.039507064133 -0.10848331064
 0.744674847129 ; 1.26950343479 -1.53854148065 1.02687477179 -0.108490617133
 0.527991840954 1.95731336296 -0.568840352522 ; -1.27788709227 0.534603023761 0.640126271461
-2.70659267308 -0.926730117209 0.753898489647 -0.0819795282475
)");
  C = _M<double>(R"(
-1.77648904735 -1.33820704909 -0.536789817466 -0.722824404411 -0.854043039106 0.934725935848
-0.532284675616 ; -2.03505707768 0.414670477997 -0.210892815728 -0.0270257990872 -0.750582614964
-0.36821401958 -1.91885114163 ; -1.33540764289 -0.45082689035 2.52002781468 -0.989349387054
-0.0325646152207 -1.73387648948 -0.245537824859 ; -0.643298891471 0.72564294648 -1.88733782013
-1.41907588253 -0.981157491027 2.24614916721 0.985960042547 ; -1.56018651564
 1.67026232923 1.58118494195 -1.67893571877 0.0377612029612 1.249774679 0.941870069776 ;
 0.439460742211 -1.28982122345 0.794482394507 -0.446620920903 -1.28819997857 1.4381369699
 -2.17319577026 ; -1.25002660266 0.0999749154649 0.254945525747 -2.45446422185 -1.33244081708
-0.00783165681988 -0.985654932354
)");

  std::cout << "Check: " << A[0] << " " << B[0] << std::endl;
  Add(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R.Copy(A), R.InlineAdd(B)).AllClose(C));
};

TEST(matrix, double_multiplication)
{
  _M<double> A, B, C, R;

  R.Resize(7, 7);
  A = _M<double>(R"(
-0.871449975489 1.28464831636 -0.174696690392 -0.893198239526 0.437978379812
-0.824546029911 1.91721268583 ; 0.882038420084 1.16883962653 -0.337564255327 0.311652116825
 0.460504019839 1.09970033181 -0.148796214542 ; 0.48340640675 -0.0429640771458 0.59633271281
 0.0123131232815 -0.703380102867 0.381752270875 -0.301987022162 ; -0.277935053774 -0.124227000291
 0.0506421182024 0.871747654558 -0.764347218237 -0.622753142763 -0.42650896958 ; 0.984756028389
 0.633533886638 2.20874102249 -0.554113223688 0.0562559769739 0.151159932125 -0.0747812534438 ;
 1.04234632366 -0.240521788204 -0.00138863250315 0.62829669346 0.901600108915 1.23382768625
 -1.4277279573 ; -0.55190554407 0.0761307130685 0.0582543263455 -0.783210559183 -0.409119251853
-0.289768964666 -0.759374770384
)");
  B = _M<double>(R"(
-1.63056175389 0.108409021505 0.271310482038 0.71595868909 0.832007025008 1.27041384533
 0.233765880592 ; -2.12017118639 -0.618666077012 -1.0905891178 -0.937510414057 -0.383438948978
 0.56182235158 -0.72757897355 ; -0.588227642173 0.365895892177 0.0121566162959 0.417154575591
-0.653022256696 0.473678708027 0.371035698404 ; -0.457226682131 -0.482036169247 -0.710168096124
-0.797202756807 1.07789649375 -0.361141931364 0.200250115024 ; 0.505129328627 1.13956663896
 0.374068422878 -1.05349371359 0.354207210749 0.653487165558 2.12363449895 ; 0.0585028076867
 -0.511483303904 1.13662653665 0.0298350086021 0.235234737738 -2.13301838241 1.62160689064 ;
 0.850805185728 0.25991765683 0.345743416086 0.432727838007 -1.1179796411 -0.327465491118
 0.734569078468
)");
  C = _M<double>(R"(
 1.42095300046 0.139267466954 -0.0473970432808 -0.639493040668 0.364401088805 -1.04751469251
 0.448178911786 ; -1.87007244355 -0.723121426405 0.368143903418 -0.292177105086 -0.176575177367
 0.617836226448 0.108260997044 ; -0.284353010854 -0.0157203793388 0.00724938797435 0.0051364757167
 0.459322862089 0.180827922455 -0.112047965677 ; 0.127079322485 0.0598819073372 -0.0359644166675
 -0.694959633453 -0.823887186546 0.22490227274 -0.0854084702174 ; 0.497429151482 0.721954081865
 0.826220270827 0.583754797774 0.0199262726919 0.0987810755905 -0.158808049688 ; 0.0609801865163
 0.123022878891 -0.00157835655273 0.018745237254 0.212087665165 -2.6317771355 -2.31521349351 ;
-0.469564098927 0.0197877165536 0.0201410497925 -0.33891701198 0.457386994351 0.0948893363251
-0.557813225293
)");
  Multiply(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R.Copy(A), R.InlineMultiply(B)).AllClose(C));
};

TEST(matrix, subtraction_double)
{
  _M<double> A, B, C, R;

  R.Resize(7, 7);
  A = _M<double>(R"(
-1.20668584394 0.784052138216 0.16042601003 0.829712065943 -0.589319992866
 0.522331480073 1.97766183185 ; 3.16395998662 1.17853308927 -0.460286446667 0.0220159469227
 0.107888368877 0.234993412491 1.14096154811 ; 0.165022253644 -0.59195263059 2.49443796686
 -1.59884804888 0.119569828545 -0.107096505728 0.424340556327 ; -0.573986273997 0.870464004417
-0.156437151207 -1.05343558582 0.468767908792 0.444847794062 1.77159891209 ; 0.70292329586
 0.00201096235277 1.23087106971 -0.0107136575582 1.75075421492 0.252525439453 -0.770620660762 ;
 1.38601704442 0.628019447993 -1.09715772941 -1.87372205243 -0.309258425647 -0.778419352384
 -0.101262077667 ; -0.588028719491 -1.74874979043 1.47425711816 -1.44562745128 1.01531280354
-1.13287934709 0.531267018926
)");
  B = _M<double>(R"(
 0.816647470625 1.26281190573 0.371506140191 -1.44024855437 0.846847085425 0.540454414796
 0.0136986668884 ; 0.441498333228 1.12236417973 0.97439245198 0.299499104382 -1.33715986105
 0.530989122727 -0.578116350958 ; -0.669202602541 -1.58646131992 -0.608082975634 0.998587800781
-0.181771025512 -2.94453294294 0.648808406137 ; 0.178319182702 0.115628973614 -0.00987996710241
 -0.919560391008 0.637703912482 -0.448961551412 -1.00246208279 ; 0.505188076647 -0.532128999066
 -0.103103783607 0.673918370509 0.296484376961 -1.54266588287 1.14802345873 ; 0.43901324797
 0.100622701916 -0.495534496998 -0.235281769473 0.221585661361 -1.23396413094 0.198172107391 ;
-0.0154278199056 0.962054577751 1.13978284243 -1.41021451468 0.370761518695 0.774255984668
 0.469991412508
)");
  C = _M<double>(R"(
-2.02333331456 -0.478759767513 -0.211080130161 2.26996062032 -1.43616707829
-0.0181229347223 1.96396316496 ; 2.72246165339 0.0561689095345 -1.43467889865
 -0.277483157459 1.44504822993 -0.295995710236 1.71907789907 ; 0.834224856185
 0.994508689328 3.10252094249 -2.59743584966 0.301340854057 2.83743643721 -0.224467849809 ;
-0.752305456699 0.754835030803 -0.146557184104 -0.133875194808 -0.16893600369
 0.893809345475 2.77406099488 ; 0.197735219213 0.534139961418 1.33397485332
 -0.684632028067 1.45426983796 1.79519132232 -1.91864411949 ; 0.947003796445 0.527396746077
 -0.601623232408 -1.63844028296 -0.530844087008 0.455544778553 -0.299434185057 ; -0.572600899585
-2.71080436818 0.334474275731 -0.0354129366011 0.644551284848 -1.90713533176 0.0612756064181
)");
  Subtract(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R.Copy(A), R.InlineSubtract(B)).AllClose(C));
  ASSERT_TRUE((R.Copy(B), R.InlineReverseSubtract(A)).AllClose(C));
};

TEST(matrix, double_dot)
{
  _M<double> A, B, C, R;

  R.Resize(7, 7);
  A = _M<double>(R"(
-0.110255536702 1.16455707117 0.483156207079 -0.822864927503 2.1399957063 0.502458535614
 0.0104965839893 ; 0.248408187151 -0.069653723501 0.189842928665 0.276933298712 0.921931208942
 -1.31026029885 -1.12882684957 ; 0.121318976781 -0.97751914839 -0.717617609338 -0.924317559786
 0.233683935993 0.402642247873 -0.191914722504 ; -1.01644131688 1.27118620892 0.289336475212
 0.84329369777 -0.592601127047 -1.38145681252 1.01862989384 ; -0.519844550177 -0.643278547153
-0.946077690659 1.08010014881 0.725239505415 1.71566775596 1.16751986319 ; 1.70134433962
 -0.176658772545 -0.969849860334 0.925512715904 -0.872133907354 1.92808342061 -1.09108310394 ;
 0.419388743269 -1.11579661665 -0.144438994263 -0.993875730161 -0.927831594645 0.0835181806493
 -0.856972684823
)");
  B = _M<double>(R"(
-0.599018869905 1.16794149153 0.73479242779 -0.695886341907 0.268931568945
 0.289327695646 1.09872219467 ; 0.730496158961 -1.45277115851 -0.50189312604 0.0253094043775
 0.623443070562 -0.467176654852 -0.911519033508 ; 0.527662784095 -0.702879116321
 -0.621857012945 1.30079513141 0.0107413745168 -0.0675452169503 0.523792754301 ; 0.965677623804
 -0.101697262564 1.19339410195 -0.219028177365 -1.35588086777 0.113968539915 -1.6504006069 ;
-1.46713852363 0.964046301291 0.535025851846 -1.0392953995 0.0400928482028 1.48345835868
-0.477589421502 ; -0.894957100226 -0.0878673943496 0.731317325442 1.11820558017 -0.376684651897
-0.403513143519 -1.46206701665 ; -1.36333317473 -0.647806195524 -0.566366799723 1.92974334085
 0.140776091852 0.716927530166 -0.251861064705
)");
  C = _M<double>(R"(
-3.22658840046 -0.0644187677103 -0.441489607854 -0.727062626009 1.71528818806 2.27700172248
-1.3308366894 ; 1.52691142649 1.9648917181 0.604293611972 -4.58998153493
 0.0215362451946 1.21021742862 1.73850105514 ; -2.49954976554 2.47443064428 0.451115151386
-1.00316253737 0.499438153916 0.481503581171 1.52197543972 ; 3.22152671479 -4.43280054604
 -2.46267557141 1.96799215523 0.018874402975 -0.40276678857 -1.46946787996 ; -3.80589167465
 0.674610130336 2.79966438596 2.29599096283 -2.46832937552 1.55771500264 -5.41179143513 ;
 0.275306422548 2.52789777908 4.6077879845 -1.69581471346 -1.83273816972 -2.10824390787
 -2.13279835141 ; 0.352566029247 1.96675732418 -0.17806542795 -0.886341525336 0.573876124578
 -1.48538702158 3.57935001377
)");

  ASSERT_TRUE(R.Dot(A, B).AllClose(C));
  // ASSERT_TRUE( ( R = A, R.InlineDot(B) ).AllClose(C) );
};

TEST(matrix, double_dotTranspose)
{
  Matrix<double> A, B, C, R;

  A = Matrix<double>(R"(
	0.3745401188473625 0.9507143064099162;
 0.7319939418114051 0.5986584841970366;
 0.15601864044243652 0.15599452033620265
	)");

  B = Matrix<double>(R"(
	0.05808361216819946 0.8661761457749352;
 0.6011150117432088 0.7080725777960455;
 0.020584494295802447 0.9699098521619943
	)");

  C = Matrix<double>(R"(
	0.8324426408004217 0.21233911067827616 0.18182496720710062;
 0.18340450985343382 0.3042422429595377 0.5247564316322378;
 0.43194501864211576 0.2912291401980419 0.6118528947223795
	)");

  R = Matrix<double>(R"(
	0.8452406966637934 0.898316417626484 0.9298168913182975;
 0.5610605507028994 0.8639062030527893 0.5957124870228502;
 0.14418085858929 0.20424058901822734 0.15451218697159372
	)");

  ASSERT_TRUE(C.DotTranspose(A, B).AllClose(R));
};

TEST(matrix, double_TransposeDot)
{
  Matrix<double> A, B, C, R;
  double         alpha = double(1.8955998718475318);
  double         beta  = double(2.9400416296278875);

  A = Matrix<double>(R"(
	0.8180147659224931 0.8607305832563434 0.006952130531190703 0.5107473025775657 0.417411003148779;
 0.22210781047073025 0.1198653673336828 0.33761517140362796 0.9429097039125192 0.32320293202075523
	)");

  B = Matrix<double>(R"(
	0.5187906217433661 0.7030189588951778 0.363629602379294 0.9717820827209607 0.9624472949421112 0.25178229582536416 0.49724850589238545;
 0.30087830981676966 0.2848404943774676 0.036886947354532795 0.6095643339798968 0.5026790232288615 0.05147875124998935 0.27864646423661144
	)");

  C = Matrix<double>(R"(
	0.9082658859666537 0.23956189066697242 0.1448948720912231 0.489452760277563 0.9856504541106007 0.2420552715115004 0.6721355474058786;
 0.7616196153287176 0.23763754399239967 0.7282163486118596 0.3677831327192532 0.6323058305935795 0.6335297107608947 0.5357746840747585;
 0.0902897700544083 0.835302495589238 0.32078006497173583 0.18651851039985423 0.040775141554763916 0.5908929431882418 0.6775643618422824;
 0.016587828927856152 0.512093058299281 0.22649577519793795 0.6451727904094499 0.17436642900499144 0.690937738102466 0.3867353463005374;
 0.9367299887367345 0.13752094414599325 0.3410663510502585 0.11347352124058907 0.9246936182785628 0.877339353380981 0.2579416277151556
	)");

  R = Matrix<double>(R"(
	3.6014691890699235 1.9143689873167966 1.0053818687911786 3.2025280346458 4.601893462203893 1.1237473971604273 2.8644721217102393;
 3.154017159032051 1.910431094629054 2.7426660575893083 2.8053613703450457 3.543552331425892 2.2851087501747243 2.449824077954161;
 0.46484962926363294 2.6473819686359104 0.9715058517176159 0.9512897535629918 0.45427035251698 1.7735134938241566 2.176949475921192;
 1.0888314590230614 2.6953366119869218 1.0838941710080154 3.927210374261229 2.342937772445448 2.3671664790586786 2.116487339238135;
 3.3488523952086657 1.1350883615826204 1.3130683843404434 1.4759916353932185 3.7881416725777832 2.8101746632316416 1.322520642012715
	)");

  ASSERT_TRUE(C.TransposeDot(A, B, alpha, beta).AllClose(R));
};

TEST(matrix, double_division)
{
  _M<double> A, B, C, R;

  R.Resize(7, 7);
  A = _M<double>(R"(
 1.5079759224 -0.953932894642 2.59746382632 -2.47049453929 0.28423073867 0.0688603541173
 -0.536556176376 ; 0.349408690414 -1.2777261421 -0.933347762478 0.464582157186 -0.328197466772
 -0.538105361948 0.949829827842 ; 1.79757399088 -0.155884612854 1.07034286497 -0.939786908704
 0.818329471308 1.10567154252 -0.544064520603 ; 0.568310061108 0.117666577217 0.299609772705
 0.696239223852 -0.980049727701 0.387373710962 -0.35677894535 ; -1.9894867372 -0.615450835938
-1.58331295059 -0.511830095642 0.363670539957 -0.124600348599 -0.688825467883 ; 0.882445484549
 -1.38543210959 -0.324424953677 -0.0788862904463 -0.190334239952 -0.273108015713 -0.324256036872 ;
-0.629503398076 -1.19531600591 1.37070297365 0.307844597966 -0.597689232203 1.23515374662
-1.17771464994
)");
  B = _M<double>(R"(
 0.778251460741 0.520035484262 -0.204623640737 0.342745129067 -0.344329851057 0.3366959147
 0.295333065547 ; 0.809956898956 0.740916403376 0.330197762798 0.584990235883 -0.665699452367
 -0.550547821731 1.28572964273 ; -0.825994744791 0.273408004238 2.52963311885
-0.264239845446 1.46193797253 -0.0826413554682 0.0621784385212 ; -1.05024963673 1.63858805455
-0.748609386324 0.649067790156 0.135091893662 0.397183009821 0.594802296903 ; 1.00829118824
 -2.00082937242 1.17214381194 0.895677024524 0.724742890948 0.325749488756 -0.23782404557 ;
-0.228539395743 -1.33664311412 2.26387865061 0.20332167445 -0.213956168789 -1.52623316983
 0.334694830222 ; 0.128826467122 -1.35113077968 1.90200441738 -0.129863717439 -1.01560250934
 -0.564600847465 1.18336560867
)");
  C = _M<double>(R"(
 1.93764611886 -1.83436116094 -12.693859893 -7.20796396441 -0.82546063839 0.204517937732
 -1.81678328291 ; 0.431391708454 -1.72452133098 -2.82663260517 0.794170788996 0.493011471776
 0.977399856484 0.738747708906 ; -2.17625354425 -0.570153801052 0.423121778804 3.55656773534
 0.559756629 -13.3791554634 -8.75005120012 ; -0.541119026597 0.0718097369812
-0.40022176876 1.07267566564 -7.25468939058 0.975302823595 -0.599827786826 ; -1.97312716843
 0.307597861377 -1.35078386667 -0.571444931184 0.501792490136 -0.382503589108 2.89636595085 ;
-3.86124012309 1.03650113852 -0.143304922103 -0.387987609583 0.889594541859 0.178942524059
-0.968811011084 ; -4.88644462695 0.884678244246 0.720662350267 -2.37052045049 0.588507045528
-2.18765832918 -0.995224672165
)");
  Divide(A, B, R);
  ASSERT_TRUE(R.AllClose(C));
  ASSERT_TRUE((R.Copy(A), R.InlineDivide(B)).AllClose(C));
  ASSERT_TRUE((R.Copy(B), R.InlineReverseDivide(A)).AllClose(C));
};


TEST(matrix, matrix_double_isgreaterequal_Test)
{
  _M<double> A, B, C, R;

  A.Resize(11);
  B.Resize(11);
  C.Resize(11);
  R.Resize(11);

  for (std::size_t j = 0; j < 11; ++j)
  {
    A[j] = j-5;
    B[j] = 0;
    C[j] = -1;

    if (j < 5)
    {
      R[j] = 0;
    }
    else
    {
      R[j] = 1;
    }
  }

//  A = _M<double>(R"(
// -2.0 -1.0 0.0 1.0 2.0
//)");
//
//  B = _M<double>(R"(
// 0.0 0.0 0.0 0.0 0.0
//)");
//
//  C = _M<double>(R"(
// -2.0 -1.0 0.0 1.0 2.0
//)");
//
//  R = _M<double>(R"(
// 0.0 0.0 1.0 1.0 1.0
//)");

  fetch::math::Isgreaterequal(A, B, C);

  std::cout << "C: " << std::endl;
  for (std::size_t i = 0; i < C.size(); ++i)
  {
    std::cout << C[i] << std::endl;
  }
  std::cout << "R: " << std::endl;
  for (std::size_t i = 0; i < C.size(); ++i)
  {
    std::cout << R[i] << std::endl;
  }

  ASSERT_TRUE(R.AllClose(C));
};