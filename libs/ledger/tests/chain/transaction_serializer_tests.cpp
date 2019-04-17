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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "crypto/ecdsa.hpp"
#include "ledger/chain/v2/address.hpp"
#include "ledger/chain/v2/transaction.hpp"
#include "ledger/chain/v2/transaction_builder.hpp"
#include "ledger/chain/v2/transaction_serializer.hpp"

#include "gtest/gtest.h"

#include <random>
#include <string>

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromHex;
using fetch::crypto::ECDSASigner;
using fetch::crypto::ECDSAVerifier;
using fetch::ledger::v2::Address;
using fetch::ledger::v2::Transaction;
using fetch::ledger::v2::TransactionBuilder;
using fetch::ledger::v2::TransactionSerializer;
using fetch::bitmanip::BitVector;

struct Identities
{
  char const *address;
  char const *public_key;
  char const *private_key;
};

static constexpr char const *LOGGING_NAME = "TxSerialTests";

static Identities const IDENTITIES[] = {
    {"532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d4",
     "18c2a33af8bd2cba7fa714a840a308a217aa4483880b1ef14b4fdffe08ab956e3f4b921cec33be7c258cfd7025a2b"
     "9a942770e5b17758bcc4961bbdc75a0251c",
     "1411d53f88e736eac7872430dbe5b55ac28c17a3e648c388e0bd1b161ab04427"},
    {"4235130ac5aab442e39f9aa27118956695229212dd2f1ab5b714e9f6bd581511",
     "837473fcfcf55431cedb2faa86644bb2b0c45f705f7b27bc58d21b160812d5fb5d0f3ef6962ddee82594e19c9167f"
     "7ad48c0482ad2868a1568258459903eaff7",
     "3436c184890d498b25bc2b5cb0afb6bad67379ebd778eae1de40b6e0f0763825"},
    {"20f478c7f74b50c187bf9a8836f382bd62977baeeaf19625608e7e912aa60098",
     "4c69f961faca5e6416b64ff5cef4c2aed4de6089738aa4fae9d378e3e82b6cceb1c3b9ca2cc6d5810489589399d89"
     "570cb2606a023f0c64d1d697d15af9557b3",
     "4a56a19355f934174f6388b3c80598abb151af79c23d5a7af45a13357fb71253"},
    {"da2e9c3191e3768d1c59ea43f6318367ed9b21e6974f46a60d0dd8976740af6d",
     "bc0e7960edf22154b581924d1281a79c1c9fe86b5dbe7aae43e12e22e43e37bfbf5e39a64d1a72ad0d9704d553f66"
     "e070b317fb00a7774bd341381f150fe3837",
     "f9d67ec139eb7a1cb1f627357995847392035c1e633e8530de5ab5d04c6e9c33"},
    {"e6672a9d98da667e5dc25b2bca8acf9644a7ac0797f01cb5968abf39de011df2",
     "e65d8ded7fe2f7ea0de5c09aa07e525c0a751a712bd8d57a7722f893dcde10bc4eb3ce6797b81aaf77c364387c0cf"
     "428695d8dffe219d92017afa6c888409c96",
     "80f0e1c69e5f1216f32647c20d744c358e0894ebc855998159017a5acda208ba"},
    {"63eff700981d8cf37605147bf6a84308437e66ad0156c7e7cee95b09ecdf29a6",
     "7795c803bf450cf3c0a12b9dfbc7b42de6ba3af998c3a2d12e38473445336f681fc296d50736a95c04da42ff35080"
     "138758a6db320a87fdea5dbd36d23c74edc",
     "9177b82b73f40bbd237ca9f99717671916263409cb9204db1c7f51b310c3afcc"},
    {"e69bd6b6a4f1269d6bba79458f0c480cd3668b80bb2ad73f169b9c84f8707848",
     "dce127843be731e3eaf0708aa5e96935c743a99d4b77ac019c40d8ca08f41055e5fc2bd4f6fa201c4940adf99ba76"
     "199a0515333dffa5e7c1af42ac86a71bf64",
     "5f624a239d10eaeabdae1f235d1d454fe5100bd7aa5d247cb07a2dedc3ef421c"},
    {"60cb7a8f7f937541bf31c8e20d7228576184e7334fa82e2ffa31f8c17f055782",
     "0c129717831e0e5de8971d1a1cf14a8ca0e6204a6dae3f291715a95d380b313fef26b3b67e36562c50273d7a1b758"
     "47ea89a54eeb2f2d913edddb9b5204d0f99",
     "adddecf78a681b70454def36fb3291c2c2a58ec9b38c37c4e95f42cc5da6d87e"},
    {"12b7d96d7444dc0557480a31be4738d17c5f9ec6d09d61d4e987349e380e35b6",
     "acffdfa26eb33bd0652009840ad6801c408788a5b540e78f413851a7600d4cddb05971cf96da6e82f3472ce3285fd"
     "40939973e682d4ca784b5a283827c5786bf",
     "cf61e3a96620776db014311099256d1f51e882101c7d4062045e706fcb40cb2f"},
    {"f12ec88f0a1165e9ff97d9552196dd901769204f45effa17bb392090e4d5e66e",
     "e3061de83bf7097660308ec87cd3fe6c9eb3211999d5814279b5f03b7ec054a8f73d74050405f9167e9fe5ab8d4f5"
     "088bd5a269662c28cbc02f6212ac57b834c",
     "318cba4e10217052ac83136863779b761400d7c0e82f0d4a6e32c2ed9045c233"},
    {"29aa0b9699eb2a295b04ed66efcf6ec4a86f4b722bfab9a08790f9b807952320",
     "3a3f06c75a491de97ff3bf892a5862647ff9988bff2dc44f31fbe5976db2cc4526d031c39347cf7311145271b2312"
     "849b31b9a977a83a6f4e29cfc26cceaf7ab",
     "f860a6874efb38b3aaa0b917fff41a5d9be0298b71aeefe8a918f4cff3694a41"},
    {"04a2d0a09ec306ca226d9376cfc86f482b1bd24e859bc4de30f706e3bc81b73e",
     "abdb127c821e4d60722ce3766779f8b027b708128df08a59c5a077a969b08fb15d2199f356529ec4a62c5edc63ef8"
     "8e448d5ed8130038867cdd892d39de8a440",
     "9aac415dae12547600f1a8ad94a264d0e59b39cdbd1234f9794c0af2ad4b48bd"},
    {"f0f643ae898e06fc1b5a101674701e5a71ec97ae1ffe3f8c12b3859c47ffde05",
     "c5cda067175485a378f30132a20552df4013b07e06cc6de5eef88b524e368d9e3f4e8a10cfc34636579da763878b7"
     "9e3ed6644626139b84f904a8f580ace3204",
     "f815930aa9248d7a83a2ee45e3f072b8b3370f5087b6708650f66430947ead74"},
    {"b17bfc6416dd0e7856c87555cbd40cb81b56db34d59b2911ad5a01dc17d33769",
     "689b19c958e89895fa0b4bb54df02ea4dc3f22753dcf3c24a6db2f991ed73fa03048ea6a1c5e2e5f699a0b71c5af8"
     "b856f1cda0431183989aba26c14f7e5aa43",
     "8495eb702f621258fded727fbd905c3fb2b0ab3e297abd2b56faa645d986c297"},
    {"68d885a28766a7eff80152e7155658b04b2bdf6d0c14f1a2a82ed21afad7cc77",
     "c32891b7caa1c1a28c5be2308b581f0fe56e971c459002a0e1017534bfa46c5e3f37bab85513dcb31c05d99a6885c"
     "b414ed4986e22ff6ab8a613ea49494c9746",
     "f13b405e18d1555a0882a5b29da87cffbb2eb41ae9f0421e4b069e96fd1239b5"},
    {"002b67aa1439de080fe8127e3348932501c50dde57474a7c56d0c61121f3e2b6",
     "8de98badc02fc3bc168964a23dd5850f380c071ff8cf0d0f34f346a5d98cabf4cc6a169a4ce07120061580dbe5a0c"
     "d9e782caaed251d5671d2af384cf8f0c4b3",
     "cfe056e6818ea6b1a3554f45991de2292c93991aee89171a9321489ac63e6325"},
    {"09e5dde3b3f672b05776a9f7c9193c2d86aa5b90f24386fb37cb1eb876addbdb",
     "884f45ee337a98734b5cb3bdc3006b3ff13174fec3369b592e16bd922454af52bb5cb1d166c390a03cfca9f404409"
     "d7a32112b1be082be2d7971eb049d29139a",
     "1056f35094f7275fba2150d90ee398b93a93c4e025706b36796dc04e52a18e8a"},
    {"3f53354160be3f4441c1fa1b383e92e4bd13a2ae382273e44cd3b03a9249177c",
     "8e9d0686542a264f3df580215330085a6956722d16b21725087ea29ddb69e74420ca28a82486f47b4c7e9d3fe0d2e"
     "42ae6d0a9612ec615ad3fe01d84656b81df",
     "b0b224383e06c4a862d18f78d72cad70784d9577d0d31978065f25627e069d72"},
    {"3dbe0cdd6e5ee4f3aa3b14b88380860fb333d047a5148e74ef2d886ce060cc94",
     "0b53b5426d9abd202b9d8ca14fb1788756e74b1ca617d6f7d2da01d82dc00a19386b11c0cc0d24b14b19d6966da0d"
     "4c928fea5a5532fdfb964b47eca6883c1dc",
     "a9472c63e5a36f7c194757f035a3ac1d2ffc1c0e76b320ca6d7e795e9313043b"},
    {"649fe46774c76756e72205d6b0a09c65fab6a151884bd12921a7e2a616c75a56",
     "61c0558174ffc0064a806e462a4221149c63f8d9f3b9616821def70a6a6d31bb2b85f5e32555f2f80e9991117798d"
     "d947c03f04662613c58bce5ee9be96aae4a",
     "1718c57c0d899f769eb55f867cb6e9bd4033859bb765ed50df9201c119131c92"},
    {"3cce78a2bd897365dda32219ade93e968fd7c27061a3040385374471afae038d",
     "746baee66bf76a6ece2e52700a1de8cc7345f43f45d29cce0a75af5f476f72ddb3cfa104871fcbafa1ed0fa8722a2"
     "307371586c30967048c568a0d5babc483c6",
     "30d5c4a6ef8bb59b0c7c12c120f56354356a8ec05a15d1a3673710321e85050c"},
    {"2abfb26c8e56b696dfb6b61471515e7b1234e69701ee4cc2195f3babf8111d9e",
     "8c2ed33dc02e2671b37f56b8802e546ac944491cbbb6730f151575ee4068f65a142462d7d5262db5fb9ea1ba67c10"
     "eed75af83ebe433fbb42450dc3185efc6d2",
     "36a04708806b4ac09c9708cf89c92b721b5b3a309e2cb8a8583fde0bf3e547ee"},
    {"ef42cdc644adb20615d51b4f6b1ea358ed0b3c961f1a79a3bffc8d07ac5ee696",
     "6471a434a365b8acf47b3095bf2a7822ee1ed34b295dc3087b61dab5de20c5bbcc788066e0d11fa4407bb7fe38d6c"
     "0028e5fbef1f0bda078d9c3073484a94c52",
     "f7938a3819c6e13a322b57febb0b93504ff41c5005994d63705c812055c8ad"},
    {"77a3e7b6d31230b6bef91002f88a5a68f299d5e4509cd4f8b19b9b523f25ff6c",
     "dd00261cc313e723cc952d7db4cbef93a87af5e87687a1e4004034d6e6a10313a5d11d0239d693b6ea3d97ba5b967"
     "56ad08b175c0196548e0a815344e3ec14ee",
     "efae388a8ba2ab4193d5c3a099b6fb9d149517828816aa58e00528dfd879ba75"},
    {"4f6dd65cace45ac117e1062e7230999faa096c42a218dd834cc2e2c5d891d8eb",
     "3b34f939a6d75a4cf5c5e601b70211593812791078959b45c750e736d9f80d04e49f0f8f29614fcfc157a2e6fd8d0"
     "4ed5a39ec6fc6e84535cd96a2160e324e84",
     "d0776b3597388738a50c3c11b9a2815239b4c9adca99c01be248be2d662c43d1"},
    {"4c1117b462525c691ba9e817a3f32777d63116fe4e90e703341a2e605a70cb22",
     "6b9dd3aac7b0a4cddc261425bfb62d08d52f75497c541a5da792900a53efd7ef69d130cb7981ee3e3a2b3baa9fb09"
     "003506144bb3998250078f2deb173d48021",
     "35c1f75c06bf2f1cecc6013dd3854614c07c66ade0de38bde9cc10523f20a3"},
    {"63500c477938d4610945d86704ebeedaac926f683c741e0e9858540d773a983a",
     "42f7ad952ae8525995a86d0bb6d0fed17e84ed21d85c3f5114a234fb90484e948f8f29a32913235daa0d129c80ac1"
     "588049cf3fc76a88b3f92b9fb92493cd8ab",
     "8f75532ddd21fa222d76b3b3b18b2e7a8f0dddb5d88d8cfbd931fea885f7d580"},
    {"8c78b315f9ab58741bab8a8bf0227ad252f29dcbcb5b151f1763b2875d20dd68",
     "2fd124090cf781de0e9b0523430fc9b4348a46cc78d95d0c706c7d37a5f97c8e37ca2842c0a9cd974b7dd8b2aefd4"
     "8fe02fb63847fda2e4a7fa3f1e975b0f22c",
     "b6265d081d19903f33726581533b7257c154c604986c713099f41ca14747d3d8"},
    {"012046c7f427b86412dfa032bcef468f10a89703f487560023bc7f5d86e48664",
     "f27c91bb41b52d611691dfd1fc607c54427f3431479208f401902d606a2a9a3af61922e169c6204abf10984d78183"
     "59a079d69b6208a219d502bb987a4c86e9a",
     "b8ed6a30e314085159d3afbfcf2ba0ebe84a88fa826cb7c144269f1540b9eecc"},
    {"7a60810c055dbba3931d78b35f6bc559867ccfe1118ec58b4d91db2f2d3e4fb4",
     "543d969e5af21bbdf5a6abaa253b48ab45d5a7a8e31c3bbfa0a08514365a42c7100bdbdd1584e3705a7111bbbca9f"
     "02b8446275147dd30359805ede4b8bd4f54",
     "d0c66a4ea0425b9d2beba68b33b70db37a8fc11c1d077a22d3e2f64538022466"},
    {"7f05d646e128826b47cbc846e3cd0f367b37873c88825ec3e0115081d3844dda",
     "da698aaf05d63a0282fde44a1df2b8b7725bb5f9d478e140c7130af1350e780296c1a23f774457fb05214144d43e9"
     "71943aa41aeac9042167a5423c0cd27ca6d",
     "260a977ee0c6d34b04edb024d3880877924afac35f5c277f2dc903cd99b80e69"},
    {"07a458bc62689d564b535051725f0bb4121c27f994b0ae4ea116565cd31a67d1",
     "71dd5333f01f2bce82b99ee45834d7140032d4ee3eb8f4bd0c6d0f856f3483b9c12af4eb052450aa062cebda3e7ae"
     "5e1a018d3767a5a9c1e5a602132be26ac0b",
     "9885ff6db03f718cb3982596ccad2f4110ffb9ab070e4d05b9c7078aaca00136"},
    {"3f0854733880d4c1c2d91b8e8b8f74bd80b125ba461059145de8cbd652a07486",
     "514071df9ecaf61f0fa833088fe4e1a58440e801871cdd2dee57fceeaa12fb313ad0a810c56083d557ec0605820cc"
     "c4de88a80b5f6b7a22d5b9caad29c299ab3",
     "dbed6be4c9967de38234ab50871205e5cfd2bfe6b36ec62fd9ec33b26123cb5f"},
    {"d856c6a64bdfcb174caf69095e4a9bb161f2f18e0b10a46c546585c3bcf5c8c6",
     "792638a296d2b5175fab84dd7db39dfdb9855fcf41a496a199341d259b2f8d51bf3e698f4d2397289665e7c7034fa"
     "0ba3e80337cde2d31cc00c92c9bc21d325c",
     "2bec534b35259417d71fd648fbb06e84715fc54d465976add4bdeec260d5338b"},
    {"2a5088e9bc8a2bcada3eb307201e45d1ae314f2c5252fcb8ef5e4c719d1a42be",
     "8872ba2c4c9df79ddf43e8c7dcab6b72e4a9dc1bcc62f5a3bf72551e9d96b0122ec3727e3447e2ceb9f6dd9eb5e00"
     "722b7840895d67f649bb589b36299b24586",
     "ab981f3aeffa0598813477d0313a7b34adc98a0b1b1950f27b0e47336fdd0250"},
    {"27778602d85e0dfab9782b7683d45034e9f427046a6426b93e7d43d044d865f0",
     "9371ccda29b5b5fc1f2386c2e584ebf2b6bce80d3e31dc4bb74cfa2af2897553bed94a6f75254fb1d97a06eff7ce1"
     "9cde3a0e667466e287b26b427b68ce09fc5",
     "08f3e6660c1f40314450effe8f96401d550f97a38de7f14c46cde70bcb25e32a"},
    {"be8202e93fa1144752db3b952bfbb63cf91dee6c5a667e595d91db2dd3ad65a9",
     "a74f3dacedb467bcd2b1b526e3a8f9b9db6f32c0719ca65992fadb63352e4214b0d5a51fffe04e850353eba28562a"
     "28809c37a134ceee0694dec34c53e661a3b",
     "0eb0f65da751e328431c273f25b1baa42695a8d72516f00c0c3319cff162823c"},
    {"221c31b897957077d7aaee513c850e5982cf02192778793f64730ed785fd626d",
     "12c3001a52aac6929d31dd33b9e4d155ba4b26609669ee953f1bb3aa64bb47eca34cdaca85a6c45ab5f8d68d4dd57"
     "c981bd158fa62534920d54210511b9780ab",
     "65e3e729689bc4878d73ce606dc79a87e6030e105ed7e7c74ed0caedd45d26ac"},
    {"611e22ba0a6703c9435a589f841f0087e8e198329c68c8954406094a2f7291d5",
     "349ff331a1bb30c1f839391f87662f280d5cb320c67d1349c491e4e21bba3f96a0a12388528d2a1c348b41df441e9"
     "839235ae200d40ff0612f283df98fd3d30e",
     "32fef74719afb4c3ae4cb1f91a9282691256a63275248c81c72d31f807578034"},
    {"5a7ca494de113de1d98548525bfb13d5ff4e44883392a44d9bbb3e094e2eaab1",
     "da1bb3c0a6891d924a8df490c7fe06ca08b72e6caf4bc304d60dba9b4ebabae56bbb26ac6293395b03bde7fcc0b74"
     "ca74599e4c0f6ca4bfc9718f7aaa157e205",
     "9a9bcb5be0deabe59289e7e56cfd3d455d1865438e113a29e754e32522f742ac"},
    {"2132735847945629433a6955a6bb8dbf0c8c27820b58ff4d346266709fd9906e",
     "53f5fb219e150a459f38133bb4d92eb9b4fcec3394db70f5cab6cf1695707497477c41b1d2bc302230f98cf8265ea"
     "c11670fc8f580d88780d163d6e35c9825c6",
     "55e07ed102e9a6da05447c49b01f978ec1b1ea9cb9bbe792949e852e163c203a"},
    {"de6490569b0a00a2cf87ac396e13f2879d9bb60b8340c078b6c28672fc574ce3",
     "0224fcb38262122e5c7f94a6ad8fefbc47a26f8aac7cda16d1e31becf57d819f848bfa6658d96e5d1289ac33d9d73"
     "6ddff0708e00ac169501a7f5b91e912ca16",
     "61ef4ccd7d05ad45a50e9455476c71f605e354adaf8528e4d9605448b1a41f60"},
    {"1d9b38fab58970d98ffbb6c90de37d1c2ded500f60669662a5bc736d36c405d5",
     "d94741f8ca6148d41f84489dcd1649919a8f553e91564c7751babf0c40cf21a899dbfac1cb908399c1ab105ebf3fc"
     "83da37828d63c49e692ec75d2c767ddd988",
     "05eaad6195ce32f1804896b86f2de3bc8c4eda03f34b990f47766d6f3faf4e1c"},
    {"a33dfbf8b922f1b7f7a3d390c29d7479408aaf09afb4b8d4faf2eab8f296f0d3",
     "7bee541a7ad1947a4e2a65cc50417378ad82d6e418d212c21dfe58b1648f365e58268d536d9c18a0df3b2a275707c"
     "46da57d1967df84d6090e128298e59186b7",
     "bb470f89ab7fee04e1911f8054815c2b6cfb9acc5ef3df1377f76cd5daf54b30"},
    {"c692e92b86827ff820d52849a7bbabc86fec95d1e58ec8ed11241bb30cd199cf",
     "d7616f86c66aa620481f4849c92d71bc8439a079ed399bc89b4a83a9bb619a401d62e2c65a4cc2bec9a1ad0a040bb"
     "65755900604e110e3a51b044a7eaef05075",
     "f56e9658303b995c641054c48b452ad1a3aa49de32dcb30f615e947659d16e16"},
    {"158f03f516759f3e29d5e1d36b7b253e824faf5d61c3f886fd389154b39961c5",
     "e1a6dca41eeac77a7989571e4ca651cce6a8d6e374f43959d6247e099ede296e1535681627130d57b88dad52a5f82"
     "392c6f1b1ad79793abdc586d071a9c3c54d",
     "8eaf7db93d71cfb2ae687fd87d4bc03591d7de7b203ac191e0f59031fed7ade8"},
    {"15ea2e7dc1c03687587217dfedc45788e4c9687b471835fbeb6f60f3580f718b",
     "8aecbde5ee92b0f613e431dfadfe7b6cbb745e7c30e3b5ef9bf9ea396b5caf0588997b76ac2b3463a829ea5c828d6"
     "1e24e4b52211874ce8cd9096e5e7eeba04e",
     "916fa18399bc1dd531717d292629ea0767361fd00b1ca57209394c8bc49f885b"},
    {"2d6c845fdb4b68abb1caf8a8c4802432e7a3362a192365c60f1f72e3a721fff6",
     "a467fa1b599185babdbadefd4afa46449a04615da5f9046db10aa897ca59d9a20e2ae1391422b4f6eaab33fae9f2b"
     "6600b8f9f8b3d1f3c004188c4ee4321e548",
     "a88b2705a42ea546baf2324e944d0b11e54435a80e50dc25ea8c3b93299f495b"},
    {"51ed5a977fee81deb58954164630f74150dc6f80f8bac0296ca4daf08cdc3a7f",
     "0853301ef02e432fbf5d4334b62c03c61dc23c6bb8da9746868676b6b96419e88eb0def012d4b11bd67be5c8019b1"
     "d60cfb6af8a705c8b1bb0657fd8b928e796",
     "9b8d76cd3ec9b625b61552b00daf04cbfb234281702568475b99a8af0c22c42d"},
    {"245b46279f91cad4cbf9d55c84b8d37d8e5e30214196bca694f0b0264116a72f",
     "7ab40c7cc08fbbaf47f4ef4f3adee324e9afeb59354b3741cc83d6c50efb4691d7e94a107c5f790c4ab150439f4bb"
     "02ecbb33d808ad1e433f94953e38d5615dd",
     "194f3c4465b47eaaa05f04e0cedde6ea473f507a66098be27c6e8c1157b71ddf"},
    {"81cb8b344703a19bf804eb29d86c34ee5ea1dfd7b744279aec27c915ce9cdab8",
     "b2b5bd5a75bab68f8a3bc8ec6b40239d901db76e0dd0288b793b4b79670adb4ad8680f3622bb1cffa1ce715219f9e"
     "029e3d78ffc3b54be6db8f15fb81f26c0a5",
     "3e0f895a46ef566843aba517e37a457fa03dbe6cf7f7b8882baf8e7a610383"},
    {"5c4e800fca72d4ade470337ea562a19da912bfe8d920ce5b79c6a626bc7ff4f8",
     "7f6f91b6a5063dbc54f7189b50a34032b9428c1fc0fa78f2f7989a68db9a7139dbf6abf8752d587d0ea249339f27e"
     "4f8557c6c95bdc840fab7a2d1557d419fac",
     "96f7735bdf004e9bab5c48b404c307a46abeee5eda6c767407cd860bfac7b49d"},
    {"475209e52c048d5d860a02155a4c2a5ef670b3c561943564843cb78dc207d21c",
     "c4e3c17ff190472c8ceb0a7dedb51fa144fb15a4c88ef87287e282a31b6346175eb71711fd943d0c0e11369f62e35"
     "8b127786f0a4e5d9cd022b9ca95a5931c6a",
     "a3ae87c45a5895f2860b88a0a93024ee07b89ac0b9f0ca9bb24eb7cffc1c978e"},
    {"1eb2407248bf94c1f1201f8ac11bf1e33b2deed96747e7d248fae1528b01d7e1",
     "fe95f3188634a0b8abc06a0c25c98137513fde6af868d351a6a45d5d32b80f3fcdc82b2f66e323fe4aa5abefcba40"
     "a70cadd0901be830df4368bc5b8c72496a7",
     "fbe866c95b62c01cbcb896d076a5e1a77d2926109d28b81bc9f44856a53d5702"},
    {"5f124ef91b05983469f3b8d260896c695876f1822a93a01070401fdcda3ca091",
     "5b832c6c98e1447b3e7d3c82df704eecf5cb80581cd503b4a7dfa5c7bd2d111361ce85f9f4a2b59d1bdeda13a8953"
     "8ae8ca47d44a37aef71951686625ed64b58",
     "6534c656ac5f65e885e62300d5a80070499a4559c727990a60c391a01cac9a29"},
    {"d424b78a53dc183baf4f0c5103e70a559d77ec44fe934d3059d15f4b0b283c82",
     "98eb2f78f1dbb5f8fc0cc5c862c3e52f46bb8916e4fe2cc11f4f97dc609cded3ef3a196c7253f1b86dcc9e70076e3"
     "3d8674f58cfedfcaca174fc3d88c42c51ca",
     "19d304b4b8ded4d229f57732197f4a27003bf423651c44973e1511933720ce3a"},
    {"3de9176b917e2378e75b3f1c81aa5b4d802d446f8ad4fbbe76bf10c21fc1ff97",
     "deff8b97622f085acfee073e7cc6829cf2a97d8acf568dec49965a5ec117d5657ed0a2d007647424f779c5bf7bd96"
     "99f7855db6f2520b449b5277d46ca5735a2",
     "debc1ae5ecbeb15ed1ec5de19b024623b56ced1fd29595557d6c8f044587ea85"},
    {"b440533c499df8d7a91fd9e904525e7c4650fb3c88e2eda8b0ee7c7f28b1b821",
     "1214583065ca7b6dd8e53ca75b90f4b3546b1592963e37bfa3381875a7f8fa66d347b613108d2a6a373ac73bb86be"
     "aaf699ff43a44835a39dbacb6003af78528",
     "8bbde1d258b6f8ed74ab2321a2cffc546ade6aa478a8edd49f262d23cc0803f8"},
    {"afed170a7bba4b3e6b9216a3077521ee189996d3fce8b69adc5da4d8299802bc",
     "a6853947e1e53db69fd44c81a5dda4c0f3e1a3090b57401f960d8d925658b942f5bd42c6da32fe7d2d3fd8d03495a"
     "be9df96e2374ca1e8521ecb718b58126290",
     "eda998b7691ea855d443deb72de807530639c9528bf17be4923bc4cb0210db29"},
    {"e89a495e0391690079209d98e8b1b0dfa926746e518c5bdaca107f95212bae23",
     "d3bc4a38d64942de2eb48c124eb2e543d7e7bf3874c7204be53baee902147d8559ad6db9f147ae5091fb9b219915f"
     "e6629cc2f9f0a437010496b03e6f279dd9f",
     "fb379365f956098dba49282270b931179649cf6d855cb923db59cff3aae3e6e0"},
    {"2ef7a33906e70bec774ac3aad46a7edf7d8f7954cafae546154b53a850da1f24",
     "7a2953edc8e3aab35de3330c39e2d924698cba3150b978372388e4c1edc560243a29f9205d4fc06c9eea46928b49f"
     "24a9464fa5269a97ef22675c9570a2e3ff9",
     "cfdd6c15dd9714582bb93fc14ed98545190f7932976d181cd5dd486e11772997"},
    {"7f8719a1d572c009847a1d161dbbc65be50013d07be943270c40d2791ce989e8",
     "36144197f0e92f5ff2fe8e7aff4387901f79cbdb27ce50a5c5e29f0436e44ab16312f8cc18ef05d7a0991a892a969"
     "782cc7f259bb5fc0f5e9581918eda1287ef",
     "dcf24c25f8c33b4feb6be99542f82baa9d045c4db88c6f61cc6660b9baaa771a"},
    {"9b163a7c613d2ffa36573de84e9cde2d41e7c73a9e4c4ea4c94676c1878dc548",
     "b3352e123332a4611101c224c389758fdde9978b81134f001fa9043baae8ea760d482b8300b9e078d1b08785d1904"
     "19a7ee9385ba7f3d5456b9a5199f20a4cff",
     "1d1e0810cb5faeb11e60deefefda79aff0dee5c7bd71d22dbedbe975afa02df4"},
    {"35ed05de35383f7416430efca80d9b8958514b699888796e542caa157baabca1",
     "fca1c11751e326e34bffc764e622f2bf3ddd407fbf3fd1f1a6b266678a4b306ae7d9f04acc82d8c11a35ca66cab38"
     "bf389dcdefb7005400cb32f099873387f58",
     "bfe329913808dc91d4c589ec1f3c17ba468d68ce9aa5573b1323f85bb9d68454"},
    {"8d511f8519dad4681067915982b1ae17b9a023026fc625a28a2684632567b29f",
     "2287db0f1c66f6715a2f0926bbe9cfe28e8b586a7eadbdd654cf3e5373fefa509a70c227abd07b04cc528d3513d89"
     "86edebe23b5800a19d0428fe8cb9502d1f8",
     "674d18ee22c8fde2c18b495dcf0f1153ca2a1225e2c509cca09387ba8d064f0b"},
    {"210f7dc0baa561c6d8e70adebbbb188d4ca6fe6bd221e05ca0cd6bf6a03a664a",
     "47fe7c83b4940a6821e29492e0d045d4ba2657323f1d1f45a35b16dd11afb03bb2ca27fa1ca49fd378ca1fd72d78d"
     "2543b117e8d53d2d3838db29c20c17ca13f",
     "bb72518cb8ac5f067904cb536cc983710fa21b3c6ebd3cc3984671927ab09687"},
    {"06f60585961d221501cd5bacdfae8c3ba307d53f19b494c99583ec898ef9031c",
     "117857dd8d71d784fd64b5b6fe1b00b6dadfc1aec27fd298bc9076c02246db6e70165d6ed6cffcb2a1d1397b4273a"
     "4d816355fcd1d452be857b1863122d340c7",
     "02d448fb70e50c94078748c98235b2157586de48a21aba822fba145154df27b6"},
    {"eb7a623ec3fecfa8a3d969b9257835074e68d0fa0915eccd10ccc992bb646838",
     "1012042e40ae9f042f9b37b8c5af3198d82d08a83363f437f2af8f425b8047a3192781ce23a3f649c168d1a978638"
     "e03e74ee83d747bf9fc0e6a587ad6b8e53e",
     "fb9b077003b7420c1bd4a0f442ff46853e62fb9da83ce8c3af7095876d04f650"},
    {"27f60f6f993028b5cbe193793bbd6cbce07acf87f000c75089319dc2a4e0668c",
     "ff29b948f4c7e095abf2b474e807d92b5b0ecf1037984c4dd909a1d90040e67c787453800e6a78afa53e2e709c55b"
     "d0810ec73a859685b434839eceb268f3e27",
     "eb5773ebdc0dcc063629aea07ab6952d4de1db04a9b45933073e9c61317d41c7"},
    {"df1f9751c89993a5fcacfb29e1c59fb93ed35cdb9e6b865087de8b65d50c6151",
     "1e4c369fa0ebb67e9216188d5d2c7bdbc187c9050ce95135e4e312d12153c047cbb26230eddfe0de5a7f0c32a85c0"
     "dd9371dc5af47b544fead8f0cfdb6ed6700",
     "79cfe903b3a7d82730dbaa88a91c4593fbe4c02ee262e5cb395b1510e3c57eef"},
    {"2ce71255355cb85c46378eac408fcfc9988914b43bf9318423a6b1d2c9e54cc4",
     "08442b0b101603d87453c65b79d022cb31940bc905c3880497671e7998de85c0b8491f6bfad00f5b05d9e9c42422c"
     "856b0eb12cf61be34c49660eb37710e2086",
     "789434adb7822be32a0c37d11d61ed9d51275a200e6cc5f2f50df23204cb2e4a"},
    {"f1e9a406e2b84d698f552d49894a7af1c51da054a7cae8f4db1404808b47eed5",
     "0137433dbad339f259f614ca873cb37c317a591fdbd762caf5a3c835e94d9a2b5732ab0e0ea9a24eeb8e392820083"
     "72912787de433e13598b7a9b084689a8184",
     "05552b7b692bd286f2c13d9fa57dca4bd5fac3df3e584eed909df2ede41b639a"},
    {"08c2b1ad6530d1fa80c1091fad80ba02fc4f4116ae34906dd608a7e402322c88",
     "2ae1ece357713d026f4b5cb1e0edd9c02b83e5fad2d714f091e94a3adbefa57764561713b4e0c705745dddfa735e4"
     "a13891d4fc39898ed39b8b85dec976209a3",
     "2d8dddad5d60780180a0ee527a45237a56aaa055606c5e9b6cda37dc0a6c6001"},
    {"b636e77cfac2541020c0f45b1efedd945827b44dfb4280573454c02a51315520",
     "1ff011559c7e12850b0950f8a501bbd9400a7c93b5b493a7257cbea61e63dcfd1ead67eac534b9fcde79b53a1363c"
     "f38f73abd3f83a232bd49f31603628dcb5c",
     "ab0a618cd5c34c09afb6210983a89342250ddb6eb4d616cc4b23921f45c9996e"},
    {"2cb556f84fe082f23060156cd84c63093befe50fd6232bbcca7d9d771545d8d8",
     "c9961f3776e34e39d2debf721cc0c45d2ef149d5fadb63a709e5be8d0d5812fad7411396361653862076ae0262f83"
     "6c18ef72651cd77a76a19abadd3453c36bd",
     "f2961e7e54fdd0fc0318b2a881be81b4ed111fdf6c401e3af8cb7923116cc073"},
    {"7950f4c455b6f65b2ed26cd9178f2b588b487bc58b0485c1fe0842a021ccaf99",
     "4b2d41db8b23c4a560dfac6a403280f36941b7015fb11accdac786c50bca650d6d5093b7530b2e73e7858ccc2a7a2"
     "0e5756f3c8845be61d670c34a52ec02e590",
     "4d7dd65d7c17a0f45e1e2645b74d788cac043e433a992387008bd46a8acbf324"},
    {"ced58626e2c7ad66af9523dfa4581635a3e623c0acd6a99d79186ae89c2bf580",
     "eb8f891b997d41c78d40d82ea65a1a732d8d1220e751a1727f67dd572cd865961720d5ac8d330e4e7aa4068c63f13"
     "49817e68f3a42c212901d644d814ad6d641",
     "d99cee79c5d742d7e10069537c722fbf5c56fb1186dda17b36639ce1d8a4ae0c"},
    {"0baccf74510f215ed36e44e25ed97b30f98bafed420f69efeb67fbb746586585",
     "b65b20312113618844dd784929c9261c9015523531ef2682670c1145929618e90f9509aa13e60a0c7a27fc0538c73"
     "15c880fadbe38f5eaa76a679bed7348c8a2",
     "69a5a2aadafb7e15f840e4fd500a9308f40c93e2de2f64c3716a5216fba37370"},
    {"98690f07d7fcc80af11aab8e243d9fbd4018765109503d8fc666dc0bc88da768",
     "4d2f1a60e490cde6abbc2a4d4a4c26d25e7210a8fb4e1fa5322718217f6a5a8bb67909b4e0bb1a1822d93799b8b82"
     "53057eeae068cf2ef67dc729ef93874282c",
     "bbc460014b0f19360cdf2908106ba48825b225b3d5f3f41715e100b8e1fb4e24"},
    {"709d669950593efcf210de6299ca83f9e979a47fd1d5d0a429832404c280808b",
     "8e71966124b79ef0d7f3360539434fc2dd0d48193a1e7742cafbc7e3f796ea0c0546c01f771fba39de049ca23c677"
     "3a7c9b14102d95b662eeaa4c52f909c73c6",
     "a3f30541bfeefb62b7be41bd1a25b9e9684304880e080d0d6ca74d18b86f61fe"},
    {"c562f282bec0f0d943c005db79b4df41a86410c9167615b46f79fd06e7a43ebe",
     "12d706259e6b9fcb6fb6c73ceae7afc05ef611249db1da82f96bfa857364f79120ddd0855395d55c1a04246b1239b"
     "b544c1933478342d5df6de990034fa88c15",
     "29ac5d803ba996484955fe09a89b43a644b3f3c97b70f129b7e6637ef71f37c9"},
    {"2f0656109426ec3676d5145c4605054c4335864fbcc95fc8efc392efd4c25f9f",
     "16a599c2cfa1de12d1b795685295617d850612f4ac056b8b5b8ffe6ac12b5aa9eb616bb74c257ec9b1bb8cf8658b3"
     "72b18425233d9b34b2ac77ef46961b2e098",
     "636e1d00173fbb9e2785c5fcfa93c946c0b4b1a9d8fd610752ce501a508d261c"},
    {"615ecfc640365613856772edafdb10a7d696b4e50c4fdec3fd8f0dd1366375a4",
     "89c7b8c754bc6fb3d72ea62c1534be6d2c20375ec769bbf571d8c3ed5df2c4de9c4593643ac2da7f8a79f6fd6de73"
     "fb717827427c35c10aeb26a72fd6276a173",
     "37a866ab12f38c74cab944a22d4ccd2e962c18723fb1a8d4af523d892fc2f131"},
    {"3b5efcd38191dc91c7b3a4d5485e4bc7f6959a311c0e461e9cf9a96684f648c7",
     "4d21971a855b2f2a40a3ce3a02abb1fb486e2cd8fe0e32bb30bcb16995d6cac52ba3163e80d5bdade516387ca441b"
     "e8bba16112088ad8d43f867ee0de7ec4965",
     "e61fbeaba83c7ec5e22c8b37012dc8c2eb908c4441549a89455ed0ebab4ec1af"},
    {"fb6d69e41c6d6b0961d65a1e16f31c9c9219c9dc3743ec4c3272fbedd6b14545",
     "bea6d8ffcf6876c5f12ce9535e33e3d1f22d07ac37277c740ac64597fcc4b6b674091305609054ba1b50957a161cb"
     "a2b03c483a96a724295e4a519ab97314453",
     "cb24a33fb0f5f532077b41abae28c70ad34ca7ee8c1683ec29c3d3e157240fc4"},
    {"81b4cc2665374c51cbe648199129539ad39afb0be62e04fc38717b226f38c573",
     "f8148c6a08369fef719b0439304c8765a045738a5e8161b50a13ba99c88cfcb79c0e1e09a81f30971831572a72829"
     "8579c9f75b48b803bdb0760a07695bef4bb",
     "f0c854d57313ce871b419c3a51e89ca9206428281491cc038027a4321aa39959"},
    {"15d6c99a0703e37932c4dd700f16dafebf46648b5aa221ccdf83efe2d46bfe4b",
     "08073a61df1170bb7f45a722269b88fc828e462bf65d1e54d66efec86d94d741a1bd07df57e194c540980c1fa486c"
     "eb4b4cee87343f58d6ef2a12a3d4b3a02b1",
     "ff5a16cf21db43a82df0220493267d6fba34ce181c2864b9d938454c908d6bf9"},
    {"ef40d002ddaa44eaad5534e3f8a22ec06458b5a1293c25dc2655f70840d1c535",
     "3548f30418556e8979c2edb1eb46c7080e8c56707cc1e622139524355a84725eeb6ee24c3ba62bf76bb947611167f"
     "b6e00e220de2f30b820a7aefdb00290923b",
     "4413ad30c94c82cc3586fd0854b6d827fcda2ed21a07e5c9f188315f3d755b5a"},
    {"5bb2701a745673c33124a017df320d0e990d79f561a171291d4d4e6fa65a8419",
     "05176c6acb1d141a3e9ff128d61533851e8b1047bb935b57a4206cf412c1eadb85bfb51a1e9bfae6f4511badcf385"
     "96f6876c795c10460260bc9ce427ee41976",
     "6ff8626d41857fc94cb9d9f60db6044ed1e6b75266a613747495b54800bdb364"},
    {"8c376cf997c31c5f22963ada5c9fba1f23898e5aae7e4598910ed69de3be659c",
     "c6b1ded8640c4df402172c7d00533f4bf3a9467cb5a0f7509d15be9c94ea221e215740ba8a1f94c6d6ccb418591d1"
     "39fdeddd5e2bdf9204c3f2f593aa5444bcc",
     "8b9747a388b9a42d62c855e5aa3ac2b8692426bcfa4967610c16caebf92994b7"},
};

static constexpr std::size_t CalculateIntegerStreamSize(std::size_t length)
{
  if (length < 0x80)
  {
    return 1u;
  }
  else if (length < 0x100)
  {
    return 2u;
  }
  else if (length < 0x10000)
  {
    return 4u;
  }
  else
  {
    return 8u;
  }
}

class TransactionSerializerTests : public ::testing::Test
{
protected:
  static constexpr std::size_t EXPECTED_SIGNATURE_BYTE_LEN = 64;
  static constexpr std::size_t EXPECTED_SIGN_LEN_FIELD =
      CalculateIntegerStreamSize(EXPECTED_SIGNATURE_BYTE_LEN);
  static constexpr std::size_t EXPECTED_SIGN_FINAL_LENGTH =
      EXPECTED_SIGNATURE_BYTE_LEN + EXPECTED_SIGN_LEN_FIELD;

  using RNG       = std::mt19937_64;
  using Signers   = std::vector<ECDSASigner>;
  using Addresses = std::vector<Address>;

  static void SetUpTestCase()
  {
    for (auto const &identity : IDENTITIES)
    {
      signers_.emplace_back(ECDSASigner{FromHex(identity.private_key)});
      addresses_.emplace_back(Address{signers_.back().identity().identifier()});
    }
  }

  void SetUp() override
  {
    rng_.seed(42);
  }

  void ValidateTransaction(Transaction const &tx, ConstByteArray const &serialised_data,
                           std::string const &expected_hex_payload)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Serial TX: ", serialised_data.ToHex());

    // calculate  the expected payload size
    std::size_t const base_sig_serial_length = EXPECTED_SIGN_FINAL_LENGTH * tx.signatories().size();
    std::size_t const signature_serial_length = base_sig_serial_length;

    // sanity check the payload size
    ASSERT_GT(serialised_data.size(), signature_serial_length);

    // extract the hex encoded payload of the transaction and compare it against the hard coded
    // version
    std::size_t const expected_payload_end = serialised_data.size() - signature_serial_length;
    auto const        payload_data         = serialised_data.SubArray(0, expected_payload_end);
    EXPECT_EQ(std::string{payload_data.ToHex()}, expected_hex_payload);

    // extract the signatures and ensure that they verify correctly
    std::size_t index{0};
    for (auto const &signatory : tx.signatories())
    {
      std::size_t const sig_offset = expected_payload_end + (index * EXPECTED_SIGN_FINAL_LENGTH);

      // extract the signature
      auto sig = serialised_data.SubArray(sig_offset + EXPECTED_SIGN_LEN_FIELD,
                                          EXPECTED_SIGNATURE_BYTE_LEN);
      FETCH_LOG_DEBUG(LOGGING_NAME, "Sig: ", sig.ToHex());
      FETCH_LOG_DEBUG(LOGGING_NAME, "Payload: ", payload_data.ToHex());

      ECDSAVerifier verifier{signatory.identity};
      EXPECT_TRUE(verifier.Verify(payload_data, sig));

      // increment the index
      ++index;
    }
  }

  void EnsureAreSame(Transaction::Transfers const &a, Transaction::Transfers const &b)
  {
    ASSERT_EQ(a.size(), b.size());

    for (std::size_t i = 0; i < a.size(); ++i)
    {
      auto const &transfer_a = a[i];
      auto const &transfer_b = b[i];

      EXPECT_EQ(transfer_a.to.address(), transfer_b.to.address());
      EXPECT_EQ(transfer_a.amount, transfer_b.amount);
    }
  }

  void EnsureAreSame(Transaction::Signatories const &a, Transaction::Signatories const &b)
  {
    ASSERT_EQ(a.size(), b.size());

    for (std::size_t i = 0; i < a.size(); ++i)
    {
      auto const &signatory_a = a[i];
      auto const &signatory_b = b[i];

      EXPECT_EQ(signatory_a.identity, signatory_b.identity);
      EXPECT_EQ(signatory_a.signature, signatory_b.signature);
    }
  }

  void EnsureAreSame(Transaction const &a, Transaction const &b)
  {
    EXPECT_EQ(a.from(), b.from());

    EnsureAreSame(a.transfers(), b.transfers());

    EXPECT_EQ(a.valid_from(), b.valid_from());
    EXPECT_EQ(a.valid_until(), b.valid_until());
    EXPECT_EQ(a.charge(), b.charge());
    EXPECT_EQ(a.charge_limit(), b.charge_limit());
    EXPECT_EQ(a.contract_mode(), b.contract_mode());
    EXPECT_EQ(a.contract_digest(), b.contract_digest());
    EXPECT_EQ(a.contract_address(), b.contract_address());
    EXPECT_EQ(a.chain_code(), b.chain_code());
    EXPECT_EQ(a.action(), b.action());
    EXPECT_EQ(a.shard_mask(), b.shard_mask());
    EXPECT_EQ(a.data(), b.data());

    EnsureAreSame(a.signatories(), b.signatories());

    EXPECT_EQ(a.digest(), b.digest());
  }

  static std::string ExtractPayload(Transaction const &tx)
  {
    return std::string{TransactionSerializer::SerializePayload(tx).ToHex()};
  }

  static Signers   signers_;
  static Addresses addresses_;
  RNG              rng_{};
};

// static data
TransactionSerializerTests::Signers   TransactionSerializerTests::signers_{};
TransactionSerializerTests::Addresses TransactionSerializerTests::addresses_{};

TEST_F(TransactionSerializerTests, SimpleTransfer)
{
  static const std::string EXPECTED_DATA =
      "a12400532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d44235130ac5aab442e39f9a"
      "a27118956695229212dd2f1ab5b714e9f6bd581511c101000000000418c2a33af8bd2cba7fa714a840a308a217aa"
      "4483880b1ef14b4fdffe08ab956e3f4b921cec33be7c258cfd7025a2b9a942770e5b17758bcc4961bbdc75a0251"
      "c";

  // build the transaction
  auto tx = TransactionBuilder()
                .From(addresses_[0])
                .Transfer(addresses_[1], 256u)
                .Signer(signers_[0].identity())
                .Seal()
                .Sign(signers_[0])
                .Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}

TEST_F(TransactionSerializerTests, MultipleTransfers)
{
  static const std::string EXPECTED_DATA =
      "a12600532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d4014235130ac5aab442e39f"
      "9aa27118956695229212dd2f1ab5b714e9f6bd581511c1010020f478c7f74b50c187bf9a8836f382bd62977baeea"
      "f19625608e7e912aa60098c10200da2e9c3191e3768d1c59ea43f6318367ed9b21e6974f46a60d0dd8976740af6d"
      "c2000186a00000000418c2a33af8bd2cba7fa714a840a308a217aa4483880b1ef14b4fdffe08ab956e3f4b921cec"
      "33be7c258cfd7025a2b9a942770e5b17758bcc4961bbdc75a0251c";

  // build the transaction
  auto tx = TransactionBuilder()
                .From(addresses_[0])
                .Transfer(addresses_[1], 256u)
                .Transfer(addresses_[2], 512u)
                .Transfer(addresses_[3], 100000u)
                .Signer(signers_[0].identity())
                .Seal()
                .Sign(signers_[0])
                .Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the entire transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}

TEST_F(TransactionSerializerTests, ChainCodeExecute)
{
  static const std::string EXPECTED_DATA =
      "a12080532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d400c103e8c2000f4240800b"
      "666f6f2e6261722e62617a066c61756e636802676f0418c2a33af8bd2cba7fa714a840a308a217aa4483880b1ef1"
      "4b4fdffe08ab956e3f4b921cec33be7c258cfd7025a2b9a942770e5b17758bcc4961bbdc75a0251c";

  // build the transaction
  auto tx = TransactionBuilder()
                .From(addresses_[0])
                .Signer(signers_[0].identity())
                .ChargeRate(1000)
                .ChargeLimit(1000000)
                .TargetChainCode("foo.bar.baz", BitVector{})
                .Action("launch")
                .Data("go")
                .Seal()
                .Sign(signers_[0])
                .Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}

TEST_F(TransactionSerializerTests, SmartContract)
{
  static const std::string EXPECTED_DATA =
      "a12040532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d400c103e8c2000f424080da"
      "2e9c3191e3768d1c59ea43f6318367ed9b21e6974f46a60d0dd8976740af6de6672a9d98da667e5dc25b2bca8acf"
      "9644a7ac0797f01cb5968abf39de011df2066c61756e636802676f0418c2a33af8bd2cba7fa714a840a308a217aa"
      "4483880b1ef14b4fdffe08ab956e3f4b921cec33be7c258cfd7025a2b9a942770e5b17758bcc4961bbdc75a0251"
      "c";

  // build the transaction
  auto tx = TransactionBuilder()
                .From(addresses_[0])
                .Signer(signers_[0].identity())
                .ChargeRate(1000)
                .ChargeLimit(1000000)
                .TargetSmartContract(addresses_[3], addresses_[4], BitVector{})
                .Action("launch")
                .Data("go")
                .Seal()
                .Sign(signers_[0])
                .Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}

TEST_F(TransactionSerializerTests, CrazyNumOfSignatures)
{
  static const std::string EXPECTED_DATA =
      "a1243f532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d44235130ac5aab442e39f9a"
      "a27118956695229212dd2f1ab5b714e9f6bd581511c103e800000006043a3f06c75a491de97ff3bf892a5862647f"
      "f9988bff2dc44f31fbe5976db2cc4526d031c39347cf7311145271b2312849b31b9a977a83a6f4e29cfc26cceaf7"
      "ab04abdb127c821e4d60722ce3766779f8b027b708128df08a59c5a077a969b08fb15d2199f356529ec4a62c5edc"
      "63ef88e448d5ed8130038867cdd892d39de8a44004c5cda067175485a378f30132a20552df4013b07e06cc6de5ee"
      "f88b524e368d9e3f4e8a10cfc34636579da763878b79e3ed6644626139b84f904a8f580ace320404689b19c958e8"
      "9895fa0b4bb54df02ea4dc3f22753dcf3c24a6db2f991ed73fa03048ea6a1c5e2e5f699a0b71c5af8b856f1cda04"
      "31183989aba26c14f7e5aa4304c32891b7caa1c1a28c5be2308b581f0fe56e971c459002a0e1017534bfa46c5e3f"
      "37bab85513dcb31c05d99a6885cb414ed4986e22ff6ab8a613ea49494c9746048de98badc02fc3bc168964a23dd5"
      "850f380c071ff8cf0d0f34f346a5d98cabf4cc6a169a4ce07120061580dbe5a0cd9e782caaed251d5671d2af384c"
      "f8f0c4b304884f45ee337a98734b5cb3bdc3006b3ff13174fec3369b592e16bd922454af52bb5cb1d166c390a03c"
      "fca9f404409d7a32112b1be082be2d7971eb049d29139a048e9d0686542a264f3df580215330085a6956722d16b2"
      "1725087ea29ddb69e74420ca28a82486f47b4c7e9d3fe0d2e42ae6d0a9612ec615ad3fe01d84656b81df040b53b5"
      "426d9abd202b9d8ca14fb1788756e74b1ca617d6f7d2da01d82dc00a19386b11c0cc0d24b14b19d6966da0d4c928"
      "fea5a5532fdfb964b47eca6883c1dc0461c0558174ffc0064a806e462a4221149c63f8d9f3b9616821def70a6a6d"
      "31bb2b85f5e32555f2f80e9991117798dd947c03f04662613c58bce5ee9be96aae4a04746baee66bf76a6ece2e52"
      "700a1de8cc7345f43f45d29cce0a75af5f476f72ddb3cfa104871fcbafa1ed0fa8722a2307371586c30967048c56"
      "8a0d5babc483c6048c2ed33dc02e2671b37f56b8802e546ac944491cbbb6730f151575ee4068f65a142462d7d526"
      "2db5fb9ea1ba67c10eed75af83ebe433fbb42450dc3185efc6d2042df73374fdc93288108ee18cb71d108f5f892c"
      "22442813926838bfb7de999c29eaf0eb8e1b3c8a003a98729225a66fb25fd86c285ebae3fa9bece950e4a2074704"
      "dd00261cc313e723cc952d7db4cbef93a87af5e87687a1e4004034d6e6a10313a5d11d0239d693b6ea3d97ba5b96"
      "756ad08b175c0196548e0a815344e3ec14ee043b34f939a6d75a4cf5c5e601b70211593812791078959b45c750e7"
      "36d9f80d04e49f0f8f29614fcfc157a2e6fd8d04ed5a39ec6fc6e84535cd96a2160e324e840456717f8757576d24"
      "7e69c51a7b83a65e2daebe0279ca83d8c0bc861362b5b3b1be7c5c71bc5b1e01e050eecf7e187b7beae112f346e4"
      "634b68ce7a781bd8889b0442f7ad952ae8525995a86d0bb6d0fed17e84ed21d85c3f5114a234fb90484e948f8f29"
      "a32913235daa0d129c80ac1588049cf3fc76a88b3f92b9fb92493cd8ab042fd124090cf781de0e9b0523430fc9b4"
      "348a46cc78d95d0c706c7d37a5f97c8e37ca2842c0a9cd974b7dd8b2aefd48fe02fb63847fda2e4a7fa3f1e975b0"
      "f22c04f27c91bb41b52d611691dfd1fc607c54427f3431479208f401902d606a2a9a3af61922e169c6204abf1098"
      "4d7818359a079d69b6208a219d502bb987a4c86e9a04543d969e5af21bbdf5a6abaa253b48ab45d5a7a8e31c3bbf"
      "a0a08514365a42c7100bdbdd1584e3705a7111bbbca9f02b8446275147dd30359805ede4b8bd4f5404da698aaf05"
      "d63a0282fde44a1df2b8b7725bb5f9d478e140c7130af1350e780296c1a23f774457fb05214144d43e971943aa41"
      "aeac9042167a5423c0cd27ca6d0471dd5333f01f2bce82b99ee45834d7140032d4ee3eb8f4bd0c6d0f856f3483b9"
      "c12af4eb052450aa062cebda3e7ae5e1a018d3767a5a9c1e5a602132be26ac0b04514071df9ecaf61f0fa833088f"
      "e4e1a58440e801871cdd2dee57fceeaa12fb313ad0a810c56083d557ec0605820ccc4de88a80b5f6b7a22d5b9caa"
      "d29c299ab304792638a296d2b5175fab84dd7db39dfdb9855fcf41a496a199341d259b2f8d51bf3e698f4d239728"
      "9665e7c7034fa0ba3e80337cde2d31cc00c92c9bc21d325c048872ba2c4c9df79ddf43e8c7dcab6b72e4a9dc1bcc"
      "62f5a3bf72551e9d96b0122ec3727e3447e2ceb9f6dd9eb5e00722b7840895d67f649bb589b36299b24586049371"
      "ccda29b5b5fc1f2386c2e584ebf2b6bce80d3e31dc4bb74cfa2af2897553bed94a6f75254fb1d97a06eff7ce19cd"
      "e3a0e667466e287b26b427b68ce09fc504a74f3dacedb467bcd2b1b526e3a8f9b9db6f32c0719ca65992fadb6335"
      "2e4214b0d5a51fffe04e850353eba28562a28809c37a134ceee0694dec34c53e661a3b0412c3001a52aac6929d31"
      "dd33b9e4d155ba4b26609669ee953f1bb3aa64bb47eca34cdaca85a6c45ab5f8d68d4dd57c981bd158fa62534920"
      "d54210511b9780ab04349ff331a1bb30c1f839391f87662f280d5cb320c67d1349c491e4e21bba3f96a0a1238852"
      "8d2a1c348b41df441e9839235ae200d40ff0612f283df98fd3d30e04da1bb3c0a6891d924a8df490c7fe06ca08b7"
      "2e6caf4bc304d60dba9b4ebabae56bbb26ac6293395b03bde7fcc0b74ca74599e4c0f6ca4bfc9718f7aaa157e205"
      "0453f5fb219e150a459f38133bb4d92eb9b4fcec3394db70f5cab6cf1695707497477c41b1d2bc302230f98cf826"
      "5eac11670fc8f580d88780d163d6e35c9825c6040224fcb38262122e5c7f94a6ad8fefbc47a26f8aac7cda16d1e3"
      "1becf57d819f848bfa6658d96e5d1289ac33d9d736ddff0708e00ac169501a7f5b91e912ca1604d94741f8ca6148"
      "d41f84489dcd1649919a8f553e91564c7751babf0c40cf21a899dbfac1cb908399c1ab105ebf3fc83da37828d63c"
      "49e692ec75d2c767ddd988047bee541a7ad1947a4e2a65cc50417378ad82d6e418d212c21dfe58b1648f365e5826"
      "8d536d9c18a0df3b2a275707c46da57d1967df84d6090e128298e59186b704d7616f86c66aa620481f4849c92d71"
      "bc8439a079ed399bc89b4a83a9bb619a401d62e2c65a4cc2bec9a1ad0a040bb65755900604e110e3a51b044a7eae"
      "f0507504e1a6dca41eeac77a7989571e4ca651cce6a8d6e374f43959d6247e099ede296e1535681627130d57b88d"
      "ad52a5f82392c6f1b1ad79793abdc586d071a9c3c54d048aecbde5ee92b0f613e431dfadfe7b6cbb745e7c30e3b5"
      "ef9bf9ea396b5caf0588997b76ac2b3463a829ea5c828d61e24e4b52211874ce8cd9096e5e7eeba04e04a467fa1b"
      "599185babdbadefd4afa46449a04615da5f9046db10aa897ca59d9a20e2ae1391422b4f6eaab33fae9f2b6600b8f"
      "9f8b3d1f3c004188c4ee4321e548040853301ef02e432fbf5d4334b62c03c61dc23c6bb8da9746868676b6b96419"
      "e88eb0def012d4b11bd67be5c8019b1d60cfb6af8a705c8b1bb0657fd8b928e796047ab40c7cc08fbbaf47f4ef4f"
      "3adee324e9afeb59354b3741cc83d6c50efb4691d7e94a107c5f790c4ab150439f4bb02ecbb33d808ad1e433f949"
      "53e38d5615dd046ce267a2ce7026f8ae7ad1afb344c59e9e19442cf418a049077787fc3c26f0f93b9212f5b4e4a1"
      "d8a9d7e9f3fefef5cbf6c240f048e1ccab1e9515eb60249db4047f6f91b6a5063dbc54f7189b50a34032b9428c1f"
      "c0fa78f2f7989a68db9a7139dbf6abf8752d587d0ea249339f27e4f8557c6c95bdc840fab7a2d1557d419fac04c4"
      "e3c17ff190472c8ceb0a7dedb51fa144fb15a4c88ef87287e282a31b6346175eb71711fd943d0c0e11369f62e358"
      "b127786f0a4e5d9cd022b9ca95a5931c6a04fe95f3188634a0b8abc06a0c25c98137513fde6af868d351a6a45d5d"
      "32b80f3fcdc82b2f66e323fe4aa5abefcba40a70cadd0901be830df4368bc5b8c72496a7045b832c6c98e1447b3e"
      "7d3c82df704eecf5cb80581cd503b4a7dfa5c7bd2d111361ce85f9f4a2b59d1bdeda13a89538ae8ca47d44a37aef"
      "71951686625ed64b580498eb2f78f1dbb5f8fc0cc5c862c3e52f46bb8916e4fe2cc11f4f97dc609cded3ef3a196c"
      "7253f1b86dcc9e70076e33d8674f58cfedfcaca174fc3d88c42c51ca04deff8b97622f085acfee073e7cc6829cf2"
      "a97d8acf568dec49965a5ec117d5657ed0a2d007647424f779c5bf7bd9699f7855db6f2520b449b5277d46ca5735"
      "a2041214583065ca7b6dd8e53ca75b90f4b3546b1592963e37bfa3381875a7f8fa66d347b613108d2a6a373ac73b"
      "b86beaaf699ff43a44835a39dbacb6003af7852804a6853947e1e53db69fd44c81a5dda4c0f3e1a3090b57401f96"
      "0d8d925658b942f5bd42c6da32fe7d2d3fd8d03495abe9df96e2374ca1e8521ecb718b5812629004d3bc4a38d649"
      "42de2eb48c124eb2e543d7e7bf3874c7204be53baee902147d8559ad6db9f147ae5091fb9b219915fe6629cc2f9f"
      "0a437010496b03e6f279dd9f047a2953edc8e3aab35de3330c39e2d924698cba3150b978372388e4c1edc560243a"
      "29f9205d4fc06c9eea46928b49f24a9464fa5269a97ef22675c9570a2e3ff90436144197f0e92f5ff2fe8e7aff43"
      "87901f79cbdb27ce50a5c5e29f0436e44ab16312f8cc18ef05d7a0991a892a969782cc7f259bb5fc0f5e9581918e"
      "da1287ef04b3352e123332a4611101c224c389758fdde9978b81134f001fa9043baae8ea760d482b8300b9e078d1"
      "b08785d190419a7ee9385ba7f3d5456b9a5199f20a4cff04fca1c11751e326e34bffc764e622f2bf3ddd407fbf3f"
      "d1f1a6b266678a4b306ae7d9f04acc82d8c11a35ca66cab38bf389dcdefb7005400cb32f099873387f58042287db"
      "0f1c66f6715a2f0926bbe9cfe28e8b586a7eadbdd654cf3e5373fefa509a70c227abd07b04cc528d3513d8986ede"
      "be23b5800a19d0428fe8cb9502d1f80447fe7c83b4940a6821e29492e0d045d4ba2657323f1d1f45a35b16dd11af"
      "b03bb2ca27fa1ca49fd378ca1fd72d78d2543b117e8d53d2d3838db29c20c17ca13f04117857dd8d71d784fd64b5"
      "b6fe1b00b6dadfc1aec27fd298bc9076c02246db6e70165d6ed6cffcb2a1d1397b4273a4d816355fcd1d452be857"
      "b1863122d340c7041012042e40ae9f042f9b37b8c5af3198d82d08a83363f437f2af8f425b8047a3192781ce23a3"
      "f649c168d1a978638e03e74ee83d747bf9fc0e6a587ad6b8e53e04ff29b948f4c7e095abf2b474e807d92b5b0ecf"
      "1037984c4dd909a1d90040e67c787453800e6a78afa53e2e709c55bd0810ec73a859685b434839eceb268f3e2704"
      "1e4c369fa0ebb67e9216188d5d2c7bdbc187c9050ce95135e4e312d12153c047cbb26230eddfe0de5a7f0c32a85c"
      "0dd9371dc5af47b544fead8f0cfdb6ed67000408442b0b101603d87453c65b79d022cb31940bc905c3880497671e"
      "7998de85c0b8491f6bfad00f5b05d9e9c42422c856b0eb12cf61be34c49660eb37710e2086040137433dbad339f2"
      "59f614ca873cb37c317a591fdbd762caf5a3c835e94d9a2b5732ab0e0ea9a24eeb8e39282008372912787de433e1"
      "3598b7a9b084689a8184042ae1ece357713d026f4b5cb1e0edd9c02b83e5fad2d714f091e94a3adbefa577645617"
      "13b4e0c705745dddfa735e4a13891d4fc39898ed39b8b85dec976209a3041ff011559c7e12850b0950f8a501bbd9"
      "400a7c93b5b493a7257cbea61e63dcfd1ead67eac534b9fcde79b53a1363cf38f73abd3f83a232bd49f31603628d"
      "cb5c04c9961f3776e34e39d2debf721cc0c45d2ef149d5fadb63a709e5be8d0d5812fad7411396361653862076ae"
      "0262f836c18ef72651cd77a76a19abadd3453c36bd044b2d41db8b23c4a560dfac6a403280f36941b7015fb11acc"
      "dac786c50bca650d6d5093b7530b2e73e7858ccc2a7a20e5756f3c8845be61d670c34a52ec02e59004eb8f891b99"
      "7d41c78d40d82ea65a1a732d8d1220e751a1727f67dd572cd865961720d5ac8d330e4e7aa4068c63f1349817e68f"
      "3a42c212901d644d814ad6d64104b65b20312113618844dd784929c9261c9015523531ef2682670c1145929618e9"
      "0f9509aa13e60a0c7a27fc0538c7315c880fadbe38f5eaa76a679bed7348c8a2044d2f1a60e490cde6abbc2a4d4a"
      "4c26d25e7210a8fb4e1fa5322718217f6a5a8bb67909b4e0bb1a1822d93799b8b8253057eeae068cf2ef67dc729e"
      "f93874282c048e71966124b79ef0d7f3360539434fc2dd0d48193a1e7742cafbc7e3f796ea0c0546c01f771fba39"
      "de049ca23c6773a7c9b14102d95b662eeaa4c52f909c73c6";

  TransactionBuilder builder{};

  builder.From(addresses_[0]).Transfer(addresses_[1], 1000u);

  for (std::size_t i = 10; i < 80; ++i)
  {
    builder.Signer(signers_[i].identity());
  }

  auto sealed_builder = builder.Seal();

  for (std::size_t i = 10; i < 80; ++i)
  {
    sealed_builder.Sign(signers_.at(i));
  }

  auto tx = sealed_builder.Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}

TEST_F(TransactionSerializerTests, ValidityRanges)
{
  static const std::string EXPECTED_DATA =
      "a12700532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d4024235130ac5aab442e39f"
      "9aa27118956695229212dd2f1ab5b714e9f6bd581511c103e820f478c7f74b50c187bf9a8836f382bd62977baeea"
      "f19625608e7e912aa60098c103e8da2e9c3191e3768d1c59ea43f6318367ed9b21e6974f46a60d0dd8976740af6d"
      "c103e8e6672a9d98da667e5dc25b2bca8acf9644a7ac0797f01cb5968abf39de011df2c103e864c0c8c103e8c200"
      "0f42400418c2a33af8bd2cba7fa714a840a308a217aa4483880b1ef14b4fdffe08ab956e3f4b921cec33be7c258c"
      "fd7025a2b9a942770e5b17758bcc4961bbdc75a0251c";

  // build the transaction
  auto tx = TransactionBuilder()
                .From(addresses_[0])
                .Transfer(addresses_[1], 1000u)
                .Transfer(addresses_[2], 1000u)
                .Transfer(addresses_[3], 1000u)
                .Transfer(addresses_[4], 1000u)
                .Signer(signers_[0].identity())
                .ChargeRate(1000)
                .ChargeLimit(1000000)
                .ValidFrom(100)
                .ValidUntil(200)
                .Seal()
                .Sign(signers_[0])
                .Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}

TEST_F(TransactionSerializerTests, ContractWithSmallShardMask)
{
  static const std::string EXPECTED_DATA =
      "a12180532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d464c0c8c103e8c2000f4240"
      "1c0b666f6f2e6261722e62617a066c61756e6368000418c2a33af8bd2cba7fa714a840a308a217aa4483880b1ef1"
      "4b4fdffe08ab956e3f4b921cec33be7c258cfd7025a2b9a942770e5b17758bcc4961bbdc75a0251c";

  // encode the shard mask
  BitVector shard_mask{4};
  shard_mask.set(3, 1);
  shard_mask.set(2, 1);

  // build the transaction
  auto tx = TransactionBuilder()
                .From(addresses_[0])
                .Signer(signers_[0].identity())
                .ChargeRate(1000)
                .ChargeLimit(1000000)
                .ValidFrom(100)
                .ValidUntil(200)
                .TargetChainCode("foo.bar.baz", shard_mask)
                .Action("launch")
                .Seal()
                .Sign(signers_[0])
                .Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}

TEST_F(TransactionSerializerTests, ContractWithLargeShardMask)
{
  static const std::string EXPECTED_DATA =
      "a12180532398dd883d1990f7dad3fde6a53a53347afc2680a04748f7f15ad03cadc4d464c0c8c103e8c2000f4240"
      "41eaab0b666f6f2e6261722e62617a066c61756e6368000418c2a33af8bd2cba7fa714a840a308a217aa4483880b"
      "1ef14b4fdffe08ab956e3f4b921cec33be7c258cfd7025a2b9a942770e5b17758bcc4961bbdc75a0251c";

  // encode the shard mask
  BitVector shard_mask{16};
  shard_mask.set(15, 1);
  shard_mask.set(14, 1);
  shard_mask.set(13, 1);
  shard_mask.set(11, 1);
  shard_mask.set(9, 1);
  shard_mask.set(7, 1);
  shard_mask.set(5, 1);
  shard_mask.set(3, 1);
  shard_mask.set(1, 1);
  shard_mask.set(0, 1);

  // build the transaction
  auto tx = TransactionBuilder()
                .From(addresses_[0])
                .Signer(signers_[0].identity())
                .ChargeRate(1000)
                .ChargeLimit(1000000)
                .ValidFrom(100)
                .ValidUntil(200)
                .TargetChainCode("foo.bar.baz", shard_mask)
                .Action("launch")
                .Seal()
                .Sign(signers_[0])
                .Build();

  ASSERT_TRUE(static_cast<bool>(tx));

  // serialize the transaction
  TransactionSerializer serializer;
  serializer << *tx;

  // ensure that the transaction payload matches our expectation
  ValidateTransaction(*tx, serializer.data(), EXPECTED_DATA);

  // deserialize the transaction from the stream again
  Transaction output;
  serializer >> output;

  // ensure that if we try and generate a payload from this tx that it matches our expectation
  EXPECT_EQ(EXPECTED_DATA, ExtractPayload(output));

  // ensure the transaction can be verified
  EXPECT_TRUE(output.Verify());

  // ensure the output transaction matches the input one
  EnsureAreSame(output, *tx);
}
