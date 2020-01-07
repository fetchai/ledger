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

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"

#include "gtest/gtest.h"

#include <string>

namespace {

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::ToHex;
using fetch::byte_array::ToBase58;
using fetch::byte_array::FromHex;
using fetch::byte_array::FromBase58;

struct TestCase
{
  char const *hex;
  char const *base58;
};

const TestCase TEST_CASES[] = {
    {"5bdde16d2f8ad6d55f5b65ccabf2ae76033ba2bb29650653eb549aac2f5381031864ec70481ecd0c6b1a16067cb58"
     "0dd558ce4e005a3af5022db7d331ebcbb26e8e92f60ee00fa1f18f16c9d0afea4158893605877408fcd029b6019e5"
     "feda281a489c08ddd38919c88317062f25b6139f10b9d8b0b583f2d8037c092b31e320cad16fbd6eae10594bae693"
     "4ade63817f4",
     "PcgC9TgGn6r5oQW3bBzXPF3vhbaR8uTwDSSFM6FovX5D5Z8ez1apmRQCAxwJYCKg8bCGM6ymjm1Su2xdKfJ8yuh4AZoJr"
     "PKRxwBnEKfdc5mVenkkyX3LPL8Sx5ZG72unqY3YZJHao3QVvUHga7UxN8XncrF289vEGf4UGN6tJRzjBUrrcx1UJzcw81"
     "o2qLmD1dAHJs"},
    {"09fc0f583d7c10", "NwquWkDBM"},
    {"d9fa765fb69f22a3dfa1efaf0ffd00cec22ce4a1df2379fa1276d902e9996ac9b140fcdcd175bd2d6b5daa8aec",
     "6PpU28WMDUuQZXg6qVKq4yuunxmVS2nngpVrtBa9rx6dUCcWksR7x95Vn1qw8X"},
    {"d133d4392c795f1a45accf0bca6c7f69ea303484ad97fd5e4e95d23d4a8e445c2fab686f714253d7631bd3962fe98"
     "1ab8b189040ccbf99f2a5875cdcd88f707a72648fe0568281df30e9a45837fef4d6e6601bcc8a35611a47272e8fa6"
     "5731",
     "HKynKtGCx9s5jS3r741ANk2F31kHTukqurWevK6GmCH8Nw7AJuhr4KgnQm18pCGLKzX7haffvJH3W1s1svbLMqJ6xg7ne"
     "8DoEBqDNUc9ynKi6QWLyj5a5CZs58z415JxFa"},
    {"d90431e8f4ab83e654533332b3cf8fe8900b9111cf163524e7002286816200b5c4e4838127d10f96be67e9e9b7238"
     "b6e284d947a0f69b081c7b45b66d905390ffea2a6dd3d932df7bf1e8aa8fdce0d5bc74c599ddb90c8ecdfae4db5de"
     "3f723ba9e8fdcbced2dc4049a892973b39546a259bc74dbc19eae3582f59076406b62e13e39b1429d19cda258a058"
     "c10",
     "9AXKirUcUWyJYJ7yuD921mJDa7v68fnRjPCGvLkm3kC8FPUAgeNxYjk2fswURzBz5n5JEn5umhYj4utZeADf28MTaf7Vn"
     "u6AUaDDTmfPnmqdirF2KoDYCT4DRVvLn93X1euLA1k4FzFiSNtBz6m55uZym7bnzUavbc8cAncNGonvdYDe31gUbAzQHo"
     "Se4SRn7"},
    {"b2edf9444be9ad2def03412d1da5c94b7268622d0fea1081985eda7400ba79c6962d40ad7908a3d530563806cffa9"
     "6af6e39cd8f1640656d697fef69f3f7ca216e20e0ca7e4f368be4128399db48d1ef796dafa2226d68967400343372"
     "7de99237078f5eb295e54cb30e3d8511d95c25f5c8cbe3f8ae140d5eb49f8d9e251e18b2389dbf6581ec9ac2eec34"
     "50688c53098",
     "m3mBqDgfsLFSZsbsaQwTk5ZAk3cUDUYu1buGhKsYoDrXzBTKWbicAhFiG47smxxe5k9FWd52KX6gwRJthf7SQ8QfXTt3C"
     "LJ2skZxkGH2EmRQo2sFcSnnKEgJ6RVzrD32qatLJb4yyyba7znAZ8cRUHkGrcVvqj9TrH28akPvTk9ATA5ovtoyo6AS3L"
     "MKkUtiXue4f9"},
    {"23440df7cfb7322a1043d1d9d0d8077f1323dd4d5ea900fd805d13f754e48b1ba8ba61df78986c163816e797fe450"
     "662767220e9120997460edf48bb5d7f4880654394054074b99a1a48173a837883e2552cfedeebd3fb7e705e61acec"
     "5c11d111e9bdce5c0eb3076270310bd99466af0ccc4ff0adbf00deafd0aa8e962db1f26ca127a7ae54a1ba51e6f47"
     "817b08a597351609aea518ae9db1ac540dc4f725efce2da5525a508e0dfb2b3378bbb1ac071f517c29f19b103e4e7"
     "83",
     "e6Kfg6TkhmRWL83wedBnddrB3tAdk67Wqp1xERFFFKrV1h1ws9XMnYumvuxRWaUrhp1EidWv2Y4AV3JsXwBLZNEbNsVA6"
     "R8TNkuDH5gWmmqrSzKYWqQGcHreUogtjQAFsEcBq29miXpSp8r2U7MuWGhRqKvJgWoVaiZxPyW25ypSDD3ddNt4TdhV8S"
     "kLX3mb6omETT3BEZb2ZGSHUHwBRbMfpTza2xWKQGJ8qWackzf2d2LbBvpYQdz5MehbNGz"},
    {"e3ae55abaae584b122fb386d3cfe50472b6f845182f337641c023ae276bb72b982c9547e3d72bf818260701f0746d"
     "647363c58ee7fbe6cf48af6f435b100c46fdb9774b1c3a3798d179f0929cf7f290ca2cdfcb8ab7a2198550bb322a5"
     "4267ecfdea078fc42da5d19d389a93defbadd7f6148ff072dd7a3dd88ab608fe06f7202bc741ea2ba4875ad2b2fd1"
     "43aa075914904aaed55c93c95cd5ca7be53ffb0a0b3fd8e09cd47f5848d8485a4808bfb7d3861c281b6d816eaa3d1"
     "6a2052b74c631e198e11c2b0",
     "5X6to6N45Vazkg9pt1WDNzumzKoNUwQwZk9MjDcpMnSqDQdm75EvULf39Ef4kz54o787EEhPEcDEGPpU5cPFzk5Zov1pW"
     "dRW5DyFmBaJqrQjhTA3uE1gkiMQEDoLxSGKU8yXu3XoR15nDnYM1yXEsSrt246dTsRwosUqhPqzFzxjnWk6Ye6b75WWwa"
     "z4Rx4oLZSu47DWLDJT9K3Wb5k228a6cqVAJvBSdAbeFxY7iQVtX4mm9Ff5ebuse928Nx86SnkSnxN7YpASMdZ"},
    {"76c9f992a8f910ff244ab7b74a971075b8d392dde8", "8JfQBNcsSURfUz1hRBGgKcNPepwPm"},
    {"a8bdc914ec0fd1fd3ef9b18742ac4414082b7de55fd5f412090094db1803a8e912e6268019c225a28dc2e4dd05809"
     "946b974b39733bf0f9aa30364db5bdf52ab9aef4fa89d5bb44808ca4397a47b401e85dfe64b6c376d8e18d9498e19"
     "381a255c7b09c9031b9c844c1a2adfafe2119aeca8faf3ba3ba3aacb3a5961fefc9218812047cdfd615828ef70dcb"
     "273144f8fa5a39fff8e8621ce54aad6f04e3a33df7121a9f1e165225eea9595c9d09c3e42952fc1cc784101acc8",
     "A7PD4g7Lq47QtH3EUEibpWkW9VDKT5CsfCi8Bz4rp38MGQpEj8asHzJU82uK5kErdqK4FFGkFSR5Ke6PcQ5dUoavoYKaR"
     "H38Z2EEJBFgSQ5RUq2UTjY24CxaBAEMmiW5vVj6A6RTAGamyJKXfETDQz253jnnfGJUbp33PRDDimTBK6D8BU4sSATSph"
     "cYZ8wwQVdryxp4JiynJuBGVBxUogzrGHgcHxQcXVPEmfQXek9mvURE6MYzRf5ggGohR"},
    {"35859a85ff1d64ab939e9e578b493f419134c91b63d30ed0b65c4fdafebb71c0dd19827ca34a7945c18daaf3dce00"
     "bf66e5b5edc1388262a72742106f326eb4200cb98f79cdd35feb4cb5b0c3fd95bbe9b9ad0e018fe2c98a1c904d0e2"
     "c6fb820ecdefb30787a75902925fd4db3e6b89e28f4dc2e282254515416f2a9140d1866a1f93676259eb69dd16059"
     "a07a0c6dd534b1884f202ffd3198b0da6299636b3094a084f581792d22905061db03b43c7a57c3bc2ed934f",
     "9bxUnrmaEkW21cdRDKimtNtbEd5TAU4aaFWVmUXdSzDG1Z2GnBrp57e9D2U6Ec7fY6b51bXReihiayVHt6Mqf3B3KkD9Q"
     "CvL7zUzVfEMV4xzQNdst31JY3rsK4QSE8siM8WmDLDztdoAoSJGmQmmv1TZdv9YJ4boNeJx4r6kUx9PzQZf8VSJjU8Bsx"
     "HCeKfeKGoy9fuuACwUsztfDB9UVMgUQH1n55rP5Zk4TorTXEax4JWbtuEvgE6Z5p"},
    {"b58d2bbd678299d456aa28c04988e110c504893968982b8c4f91c327c01c38835b442d3608183efaac924ade0b87b"
     "f9c09374270142b7eda56c553fc1d6f544d37e997bad9f027c21309ff2461bec1f7163f84bc11556bcc3646f64e3d"
     "da140ac8327990072bf898a3a8c36a580248aec947b8c18e0dc826edfe0d01e41a4101d8bf90710e633410574538a"
     "a9892e55dc23a02f470fa16390b751d4b08cbbab6486d2b25e8616c988aeffc0d0595dd0e3e02f5af62e2aab04bb7"
     "852822aad589dc79",
     "YwU9PEy4VhsJDTnBpZjKSxuoyjUjUN6wTwsXc6Ey5FJHbSxdzLE9cGhEuYC4XYbVW8MJQQjX2sdoVxTMgBpz6LjnmRRLx"
     "S9JcCLpCn3ewxt4bbjASZKhcERRnNEYhJjxuYtRHqXK4bgp9frEnXXzdPysCqLH1ez9JGuvNxzkLeE83fnKuWWuFEArVu"
     "yrp8bzFUnirugVsmUqDGUuqyJqPgPdqCskda3bnSKPHGNdwsHeYMzi6DSaKSB8Crw6WrURmHv3s64WL"},
    {"ffe8b157a9582c2ca3100239a3cb581fa4b1b937360a8a1e17d2b4323aeeca4fd3760789558b0d11b60709a4d783b"
     "c747e3eb28f2516a062bc6d7cc2ad3ed9453b56a0a26aabb5f826931740ab4c6dbb7a0d753543b9dc2074cf2df1d5"
     "3d4f9e8c3297272d167cee0bc177669a09e94014301bdbfa5cb5baf5a9aae6fe89d1b2102ba0292e2d316bd309520"
     "dc68ebd5ccacb586a631feb47bbb1942081211fdac5e7bde39824b6a628e8abac452e1ec99c9a88e0726ad5855e39"
     "590bc38dae215b95b2acc44a730f7a1c2e1ac900690aa4a1d0d56cd8d933d86d14d41d8577983a16a93ef22aaad6b"
     "92279ddb2",
     "FdzWbiyZaaV9m56D1XZsbr3Lj1DZWPZ37pRUkjbUsPAqUXHuPL1n2ywNDUipAWXyzcJVUQTgpPFDgMnB2RQ4tPk1as9hy"
     "ShcncdgSzww8w4runvM1PEbXKiqkUJiHh1CJoKbBzkvzGnunSEps1ziigAs9WecqfU1vJQ7A79h3Uah6tLfQdGMT5LRJ6"
     "XgsdcYjcrk5myLdhbEmJJqB7zXHZDxveXS8tC6o9D5kXscLHTdu4YocgkKmKh8LEdGpXYxFvC8ezWRemcBh6yJ1CfXkC6"
     "hE3oJr7QLFxU61qeLtH6u9rGtoMkjbqHk5S2kg9gGrZHT"},
    {"b3a36427f94fe3915aae19a44b0fe81977e32a9de4948090968cf7be2999d4471f5495fbb22914c940df7d0527921"
     "d4de2495000177246e29448839c5f62d9fd4236a6f6201bca5ab417ec98ad3ec839c45d6626edeabc8c7db8e3fe99"
     "8d70466f2a9dd9edc3dc29276faa720632dc12d8c93bef9d7ad06e868e1f5a7948ccefcc1f4d85104b27f2c1025a3"
     "d14062081315d5c14221a0d2b2417c66ef0fdf3075c66402e18bc9f9473436b10a7c46c671e3a900a02b3d4184c78"
     "99f57e1ec95172b3adc97a659a1bb1e8f89a732a68c07bcb0c86fc276efe5715ae78057f49eb994f298e468b44e7a"
     "8c89cee2acd6812",
     "GEY8cbAfRP7kTkCm9rubi2o2qvMjpx7rEMipMcmeP2BZmMFAsjKixnLr7uzN9cWHyKM62uanqbABnReUmaeFqjHDdt4bY"
     "V26UzjUrdSt5mNFLGn1e6SBZ6hNEPbVVAtoS2hxxbxnUU2LCWBjCPN4KJkVa54Zt93BBtT1i4JqwaTXb4jE74WeSkGdS9"
     "RVEuuvfGxt9bMnAsoaubXFqEzgqjQH486PSceTBYSKUpCEpUdr9krdfGJzcXmJB3sx4SUwFaeHtTtLczoUEo1UVr7EYmo"
     "Bx9b1PCxRiTBjq1TdoUp7viHqdqDWLp1S1Sn5jiBngbt7cyLm"},
    {"ed77e8f0543d8e39545029119c88e289a3d11f8ddfbc180157367fd5dbd2cbdd73064c93a42e162508b8ec0acaa11"
     "dcace6f1326798b7903f2a2f327c80d3f8ed40843fe9d6aec40b6cffbd2a6eb63d6aed30dcba2e50a191276ff294c"
     "c0ce784de9ce1eaa93922ae0af39c3b1ae4cb9d1",
     "4PniJi8p3LVAwig2XnqU2epGMQ7kh9CZdt6R8EGxTBCrmwoNKkQ2qgSc4H6wW7ayK5iV41Zt1nmjiGxmU9T2y9JBkuMap"
     "y7x3KPWYcajjgftTdg3t5STm3pcpYog46qNtnAzTfpq5LEtwf5DxeHpGKp2Hgc"},
    {"a06ad57560cc3d44b918ebe0e512cd20633c927652aa677bc28f7b98d2f933f51306015ed6fde938f980c97c68111"
     "12543e50438abb4c161c2db5f373805d69355b9df63a9f00632",
     "3NLHdMhAFzjZuBpew5DzUc1mx55hBLWnk4wVb3ZRitFpGX969ooBJAa7PwY3Ft5gh4JB9H2g3NbTT9G9b5qVLmXTz9Djw"
     "G6nUmw"},
    {"6e0cc10580f5bf9ccf5dc0f0d96afab02364388de47b552690ac06c5db03eb95ffcd84893b2b2ce4484df53d53820"
     "e2c0b49d65fe072b47fcedc001c82245ef8f6995568c691ccc51341bf5cb4ef27913b5b2c6c33416486a13f6ab270"
     "24359b852524ef44749c9c7af900285ed08487007e3ef12bdf41d8ef38c30c0920f39accff638eccdab3c468e068b"
     "c425732e8760594ae46162bbc04a1f766a1530a6623fdb611bd50282209df1880f6d2f48a4e7ca8d3b412803ba803"
     "dcf17556cfe011bec3b23fdd5ffbec61545e3786f2d84d4e0baa419462f5ec53c80b4c7a88dc50379a48e3263d474"
     "e55748824b4a0ea26c684582e85",
     "MWga9vMHH9bNsRxQxjA25w3SMHRboNETfLc2QhxJhHd1y8ec28h67KEknWoYGWVNazKXnbjKK1xDuFPD1vWdrdw1KNptS"
     "2R7y17R7GrjUC2zaTBVSAbrL7LLvPBKJr2LPrjgDyRWG7cdNsGeoWWi2ukCkQdpEa3gcWNX8URrkNLQNC6YbGgtrGbL1J"
     "6FmJT82wfGdPrVXxkNL3ZVoo7oCNorWLNk8L8d5qqWCet8U8CFrj3Dukv4dbHhNT784ezaYArRPUGtgs8t83TnNuRtit1"
     "WsyCTSihS42uRimRWFgwGo4t4AkqAMj7ZiRKLrP7tYDSWHZWQ87sky3ek"},
    {"6f08071aedee9c33c1a078b4f4e63449462c0a89060a5c655052f9433732a09aa877eea1f8738adb35ccc76ab856b"
     "e42fd3672ad32197f07be8145dc5012b81a3e3ec7",
     "4Ht8X9YRSTv4oHFpEzpAeexzJShneRaJJRrUJwVgZcjDsSwXtGvrk4T2TRWxpqto5nkK7Ax6WRtqbVEgiBEVuLcYQHB"
     "U"},
    {"01ac7394a6c90e9264fd673ac190bc66d45e32f30721916424d4322c5fd65337d9cac07689ac593d3d5e3102afa05"
     "e9b729c656b20c4346f744b6bb5b6fd250273ad29926aa5dbfe0f80576fa1fe4d10fd6aef878ea6bc0d1befd86ad2"
     "818b1a6b70d30bd3652dff7f7661216e8237ae13a0c099838f72180cfccd08757561477990bdba57dc31f9861dc1e"
     "bb980d85c6e4391f26bea8a79ac47cb295f20161c0379e4dd88ecdd7fba0087fb6404d4e71090c497b12cf3eee6ff"
     "2fd9223e53c189afea9d2f738e588ad111ed",
     "5EcTZpgr2EYqYBEjCmL76LPHrUs42qwBTPyURADEnhmvwsqRek6vvuEULUfeqfohBes3Aup2ntTKjTpEedVUy3cTxLid4"
     "BxTi7Cs126Eik2WiWLweTQnRFzs9A2H7LyKzemeFxip3oacwdagfXWgNmBDQTPfmw9d3StryuXZwCgch1Ujt1dt5bNpE5"
     "ejj9KtLwZfqCUFDz1a3MShggLciuUSgc6PTdrpPzAfCsXKdaHaX4tWcgYLVnqxynAWDWY5dvkwwbtLx8zEXE3F3tn19"
     "e"},
    {"176f7450dc54c402", "4vMSaeNCvCh"},
    {"4d42a510e61e894fcb45d122fe2602e2ccebfce2882c6835ddeb8b33ebdca4a15cf415",
     "8i8B6iUMQCv6yXz5xc8jeQ5VnU5DHs34W7DBx6ckZigYEgmA"},
    {"bfd08ceb1a4062fd9760cf3e91c48aa006dc39f51372e27269342bab36185c4f7b48aa1f9c0a35ec9757f5",
     "F8Xiq5ZdmG5kYvJqQEerJzug9giNbQUDPFpPYJX2XqFWHQtYK3FT7eRJnkU"},
    {"c0bab6830dfa25e3724fb92d983569c1a3910e57af2700d6b931013d72dc5e2e6382a377318b503cb437dc34d897a"
     "3f620c0768d026bed65e8ed0c",
     "8joVQtbVDreMkLYxzunAetYf8YWgGzJXkn19tGPxBqMqPC7BAqWCimt3ba9UaA3iqweVhC6Gne7Q2THrs"},
    {"473ae08c92dff249817eb05cd848e05ddef47c61058e30ce71a2b7149630b035fb4a13cb82f4096c386803fda625c"
     "22b0158ec5dbdbc490ce78f04102a76d1b3e9fa976ef776d8ca42682d19",
     "7t2i1mzKVy7u4LsKD8aV1fKQ3zqtUD3WnW7UmnHZ4AA1wM5AWMZJXyFUFcd7rRDPFuLRwKwq58CWer36jAcSFi4nphzDw"
     "gxou5eX1qFn"},
    {"8b866568554f98bb7fc5a26b28d99f620c5fc211eede53a76c60b01bb233ed64161e7f29e52ceac3e5b3c4bd064f5"
     "f08166ed2e2c524d7ae5bf1f0044ac9d02d3c67801b6b09a1279c0573d941ec8883e42390b61260692580c3674098"
     "5542c86924dcde28fb772729c4ec3cab83f3b7a266ed68c4b79466428b1ea5e30f70210b62c899124f9960ce44dd7"
     "0e29654d98c75e880c9c85f56d245028b325ae58fc0fa9de0de30815a362b0fd237f83cf2bfae6145c3fbeb99319b"
     "a738c767bfdb511124cb0eb07bb53c00f7900fd60bd95893f3df6df5f20bb74d7f6072322bf4752635821aa9b8151"
     "e63ad46262c0fa34db70168b4421600b5c4446b57",
     "5MHbhhwTVDmJjxwDmri9MR4ydiTorYRZHBoAcxvNk1QvMANammzTEUzSJ4LJE4WviVqdsFiDTsLYAQdx7RVKNbvHgiwed"
     "7zFT8xEpXv9kt2RjkmTzysAfH5Zb8iRgMPqwFjQcZKhDwQAaQxdYp6Hx6JWWWcaGnVYLN5LbANaWPXnTDWXR83wP3K9Nv"
     "dJJc4aSNzMfwZSiPfG1Tgdi8PeABUoGkrB61yGj2cnwpXZRxgMS4ejATS8cjVk12b51hMC2mE3DAuzdK3NFSsxvS6dDwK"
     "bUJxeenpwxW3irkbyqpX42hSeu4Yr2ptiaqyjLcmMtYAb5ySv6iBugaNi14hpm2z6ia"},
    {"5b296b72064b6adccc7631349089799342c90f341c15857f63ad01ad",
     "wPAUA1qhjn9caWwVmiCgBVBuogK3wCAcRE7Poi"},
    {"f6f9a386ea6bb2d2441f3c98021d714a2d908c9a339375a3a7b7073e056d515a5fc691097b6d55bca3751040a320b"
     "f9d6e433a5dc8d06f3562a772d974d65b06b60985a499850770a234e4283e4a953779234c51b375c1b60afb99e47c"
     "ec3a03adb25f138a929429bc0e7e4b9c43d16c093b398fc217bd3d612d87d476e73aa0c3665f41a77bddde25f0406"
     "b596abe885851bd71c20f0b6487e3fbb3ea00",
     "PMPtDjUUxhVHZbsjcJWBh3CVKY9gANJQ179LDZjcZ4kfkfG8kz2BZqoZBB4pUMVE7TNefAqTdU7FDH5eXahLvqwNHfZyG"
     "xST8Sw2q4krqL97Ncpcf3TFLcRhkyVrj6mSZsjWR7TSYJ4p9QaNeK9E9T17JZCex65pjw7pRsu6Ch3X6gkwxZ3eUtWk9u"
     "NEJbMTuE6FfMHARRHL4ictrvHvDbdZ"},
    {"407c3d23ff3a961824a0df3d00968499f1d9556078896ef306090fa9f672cc274558fc37d049eefc488beec9582c3"
     "b89259af7981cc27fb4d1f074c229ff0639f7de15e3ae64ca221c9d6a0d6b95b550eb3e569cc7680911c1efd5266a"
     "fec5476596ad3caf9b335476cc611c224cb4194e606f86fddce04d54",
     "gTCWBxsJqVDDnZ8QqJjpPn4fBSeVo8q7LYPRRt4caeTFNdcrE2cQz8SpBhMX1GekEwoNz9Xo5MTBQQgXZcXVJE9b3sPCD"
     "zWUgSGLJtFrr5Pc3YkBTTQDQ6eZq9q2W7LjSJDuQTp3NqMTsn6eMhQtktAnmerHp3sUdpY9q"},
    {"b3a128f291d075c4659f5da231a20cb49114dc5f955eee15dc1c79ecb99f149479f143b17ae5b332f24c6c41fa7c2"
     "9005317a2cda379efd31e3738a12cb8a13eeb6cf91391e35668ffefadc3d428d82e3334d5b6fcf3b9138ffa955234"
     "d94e5d15f8f5c511448077a5f132571f1b76f7cf03984c30259fccf1ed22e3fa8acad32b398748c6d873f68715508"
     "dcc7bb68f53e3ea8e6a30d021455e3dfb56cad050bb4c7c9cc0b94cf109c9e1c30a7b7d524ae94dfa634adddf76d4"
     "1826eba430ef99c2da0f85fe4d4052aa7e501f52ac493a77a182c3f44997f94e44e219d8e89af967bf4a4f83a9f8c"
     "3779cf5b0",
     "BGvyF9QfxarqEdsKdkaZJ3nFnpg4Q1VMRYZxh783Qe34Ysft62rJTu4U12q39T6RMr6Tf2wFo8Dgu74wgAQC6m1ETGrhx"
     "51ddQa26iLjjSDCVQ3zfNFMW4TudZjdLdBo7KgKAMvamiQxA1jwXJz6UkbeL5fddzjvkzedoJby6Do6ZpSCy6cR3ypJ5e"
     "VDHWM75mJR4UawqykCvq1gnHeD31HXfbW2huuSTuc3AFcyNa75CcVeWKYS6cD8d8WXixnnJYWX2DN8CkDNARXzfK3i3dy"
     "edpavvQ4tyntFJxYELGfbD6BFkUhqWdSKCfmbVnMWC8Qw"},
    {"273e5180b239b1ed7874558f7649daa1fbf7f87d6e23ad3ff203b4c8c9c76a9952dc5da4b8aee914f68dd5f2f4ed8"
     "0591c0be2900df775bf2932e88aa1f12121bf66e3b693f92ef550c84768549a19c6271f5aeb7b617a357d08a814f8"
     "c2e9e3be4ae8075fb1c8b340b7b57a49c59dd20bf1660c20ad38c6888eb97e960f0ced15c790f5995cd64fa306931"
     "05e37af5a4c3bc4b4cfe6d36520f8610a2289cac8a0ca3c2e5d6a3c7257b5ed4f973dda2483a6cfa6b260a0e5fc76"
     "35c0c854c52da048b0fcbef71f9ea877d31ddce1b4c2fc8c9b1fd1df9e71d771114d677ea8110e3c7cee5c8162",
     "22EZEkTrhNdDKDsL9rRbtSvKrRyGPydPVSaQdprmCQVTyuyX9WJNMwidG43LFjGiFDh4JZkrn397PVvdpNMPUVvrKMLn4"
     "s4Bbh4cn1H2wN8iScdnS8auDtiGyAzxpETf5rtKd92PoGd2kmbSYr4tX8yRXohz2rv5UoDrw9DLpYSLhv1PnyUYPHKjR2"
     "b44uJ7gG4aFYvynFixoPc8RXa7bP9RpMrvXmpdkeoeZmHswfwp2CkDLRNHvtxkH4sXUfJHmQLe8oqtCyD4anwooLUGy4j"
     "fZxZ9azHYU3VmdFvY4vwCToL9qE8kFAFWyqcd"},
    {"7d995df9ddcf09a2200b72b89ec73d1582dee67f27f5795436f05c6a4bd8166aaec045dfd9aa53d050d2365761c15"
     "d51dfa94fd22175364ca4d0cfaa2a96f88facfc149e52b831ef95d00ab1f873f9621adba8a27f7e601986412955ce"
     "f5ead78203f9665402a079d7cfec01603abd144e63",
     "8vQEeKWThH1YCUm4ebtMyyrmQqB3EDxBTJcqbiwoT4VmdWFDHwZ4RRiewqFrQD8EDr254kpCnhUEw9awsuJbdjWnAM12J"
     "bKC6pNgVrbaSBa97yiBXWgWCkXTjxLzQEWQyToBi9oe3wt43jrLqgN22qq9hVvr"},
    {"2a0543b38a48deaac6a69c84609e4df4c4e5cab98bb306f756290d3d886c5e90010dcb2f72f85bc4cfdb41907a575"
     "159a56b89387f61cb712817b87a4a79703f5a67d5f12229709395f944af5de6b859d1b6754afdaa0e41c06c0fbe47"
     "347ade54e2b120d637f91d31db6c5a4177583b8742bdb244cd1d93",
     "6ppSmwLuRDd338o1at9kSUpQ9zVQ44iXUNFSkebf1u6W6W7UaMqRVvjidRiytUotCgQiB4iseNPSh5Ub8pfXuk31nGbzu"
     "siEHqe9rGCKbZhC69hAUu9xVraCKgQqqovV38PNBZHNKdwRA5i1qExNJzcSzoo6Mr8VeSDp"},
    {"f3bd32a7e6c4f5dd4c8e8d9c4d14a672e1b0cbcd23a258076d867926543696a9772545085dfcdd55a20278611215a"
     "11abed9168dced1376dac",
     "W8oUwmwNzTspqFrD9of8SoWLNx2wGUZEBmNkSRxjd6F8FteVPmLc2HxWDpkc5nf7wTwJhBW9WUVZeT"},
    {"f9fd3fb2b05e8380b6485ac9682c01a2c8ba20c16487ceb589111dce5e037ecc3b5092a07932bbaa5b70965c30d18"
     "e42b13248bb3277a5e77322c6eead18c53a19071499088e4d0daf6f4ba0d48bb6e8003811643eafffc3fce0d4355e"
     "6658f90f0aa3f20e67b7188ddac1230575ab7553e3afd757b4b3d017dd7d724194932ea7754902df50f98bdc86dd2"
     "fffd4202c214d4bb8b6ae56d8f4f52d0ffb7ade073408068ed0335ebc121f55cbc921d0829d3c81a95b86105710e6"
     "be540fe49ee3124017678323499d80f0a71508848641ca4c4fcaf2ddac2412967001",
     "6wkUNswCBnKGgerMRuZWNHRPhCn4uw1Wd7s3oxgwQV5wseRjH785Pa73ohtpXQQoL4A29xesF9RgwB8tCsxArwJCBdqGx"
     "saoazKH9FgrhKwk6wivdvM3nivMQZFytCtcsmTnynaYq2CYvzuG3tuY9qFBvKhoyRpV3BKe6D8Kx9doB6dhG6ELgTX8MF"
     "yGtDBpW4EoUNKrXSzmWgadrJBLNaUXGAugxADjYhtMBmH4JQbEa6CzoGCV6sr434m2Fne7SWSDHfoS2DMLpqVA4MmYWcT"
     "wKGfHYXR4uoauu9HhKA9ZS"},
    {"34f0b1beac51e387857f30e38e3b8345ae14c734030468d9641d757db0c06514963337305af0fb3d7abed74d0db89"
     "773c7515760b7cd4595ccd074ea54e5daa017135924af4d1ae7975965f1807f18569a70920d32ff01a0db1cf39bbd"
     "6c",
     "wJ26U3qVKQ3L77D3yKks6ZCyHGmWjLBXQVxJussr2R61B9nYV4RH7n3QsndSUSW2uWJJoeo4jkpEQPihhWKsD2zXryPmn"
     "DKEvPc8J5HB2rq3cg7mTfS81ZZsGq6GNi91"},
    {"ff7c46363bb0f3138068cb6fa2f0ed30471cb403a738b8ee7ff6dc064d47a6396148db9a46917382689ac0e50611b"
     "f3a5aea4083f4274517997368b7eec7bd08dfadb585e013f9df4b09fb80acd024189ebe2af4dda72c1671f59a6f14"
     "66dce6faa2a4608356b7f082bd4d84ec97c841ca6f9ad9ebd83895ece5979423af8dae91617ea6ff9021dd328890b"
     "e7de7de0de0e22845973a2dcca9fbd6421e273127d6b79d4bdc564f5aa92b05d3f3f148c9d32e84096b89dfa44c4a"
     "be158cb588be925ff60ce381c48f29a859cb8d",
     "rCENsFqnWGAJfx3dd4Cn2bWeTmyd3pYSHEVZ3d6PRd4mp3Sw1DVUtBKj1jJjB4oLwF1Udups2bq2AHcQpMkYub227Cx99"
     "Abhbk6yJkSwP9KEAU38tH8dgX1bgMErNY3mGHXvEzMEi1XsSyjdY9eh5NrQQViTD7iZcrTbC2xCBErCT8RQrZi4f7LnMi"
     "zujf71LArLHR5w7vaEtSPs6GGwauSsinJAsqU9qmHPHK4Vh1Bqs6yxhB85hPQSA8RXjaRdAEQGtrowEMx3BW6sjKFSSJZ"
     "r"},
    {"65db15e2631b6091c49d6a3a5d7b21e24c2f7e6f85ea364d6a3a5cd877aab0b9b1259deb75506cf88be42c1f8781b"
     "7c30914a6d8a8d25f0ae018b70416b624616b45650090dca62cc621f78d04e2cef8345bd844fdf56743dc4d23599a"
     "174c7520b965621393d3c5f43c91eda9322b552405bd6fbabbf0f41f0e3876c4eff2a268ef9d8112639984e07619b"
     "83292ae9861c54038fbf36847dcbbd3828e9270427e5955533f8210889c95209d35d341862ea7c1c5555d1f4b3007"
     "bb75f61b949182fbf6a6",
     "72616Pdb6Z14bGks8wsNaiwxvXakHVxbrcX6V8rJXr3fKwAXLM8VbHkBjx5p5iNY6zi2ff9s1zrTvpFWQ2eFRwqtP9Rbm"
     "NQTxzTGB1ZQouwhUg9vcbT2CkiRhnkvQHco5ebNCpNZCCvDWWtshGJb93xdai4a6zxQjkw8GbtpZ7RrQgtRjL2TGyNFcy"
     "e54v7FnX34AVzTayD7vf4NJPapH7xaVjSqrQF6zjBWyhRooQmHruSWwyUgU4VafJWyBXfDsTeUSsTExbhf"},
    {"7419fe3ed0ae3c44a1821fb1182b839df215057e9aeb95c4a774",
     "4ZCWTY6XV2TEdZTcz2nh1LEMRYxJRqFw8CMh"},
    {"06e6f19897ea40a0b50ea2093e6ae4ccb6dad75aa65af69a79c206c1f562fb3d062ae6ce857b70576c3b8550610b7"
     "f9951dffaff6f586791603c021cfb11896e5b1a6fda25fdae6833dd2f4c1fd6612fc150315f3de6a2bffd7471d823"
     "26344f069b03338468b2488366f77943720273ecefcbb1ba2b86f000c10cf862f7a924a658094da158d0534de302e"
     "be082e7c4559560a6e175d279c01b4b1be89966170d8202fd85bb63f8a3d6d632eb9e96693773c922f346418b1c48"
     "0bf69a52b0b722b3",
     "2DRiTqNfwV2Z1pRAY9TXYNC2TBwHdqsyhyjBA1xvh9uUAtDo49G5RS5nMirtpSDnJEh2WiEuJfa1aJkbQpuThGbqWM22q"
     "pqJqSs1FGNpD4vYnspWKZTbPrqjbqGWUbwhueayDqw9hwD9z3Z99KtLFyJWiftXdZbvYpqQcCd2abUCpYKRYHTKDb34No"
     "Z6Kyh48JCA2MNuBmNJ55uE1KbKKN3J17BJtjexRuLn8eZ5oVtgEAU8QAuLNY6mwQ7avUiTELgnxFTqk"},
    {"db5f484c51476ac097edfc4145f3d7bf01940c32", "44G8s6ee1B9qEBNcGZErzAuUoVmj"},
    {"ad8cc8423822f8c110d7b5245c46d12f43a8f4a32a4cab290ee0b8fe81b7f5cd70ee7e7cad704406bc55d6e8697c2"
     "40315bdb6118375bd9d051103af91315e7e6e590a7ab91817656968b1672ff20d8ab30c01596fc98d82addc98901c"
     "7157275b1f265172d9de",
     "AzybGfrmCp8sg1yqwaGPH64wRQM2pmydoSgxtvryM65oXDotgJSbM9yZq16amf1PRDxCy8orYQ3vrSJHqixEvBJXwrABj"
     "9MTAcnaUbREDu3qxAVM1AcDaxF2ZoCRbCAQCbcX91op1PFoj"},
    {"ea21575c88946d3ab7d02c0ec8a6c8b5ae8045112276b6be62e86aba16c11fd950734bb2b27d29c436fc40ce6af0e"
     "7d763a292668fbf0ce5e249b946d39c67d2aa055891343128df0ee179ef986a07651d077db02006b5d104c7234469"
     "97c701a6a77e5bbe1eed59ef33dcd975ab22b02882a9c2ca263193a6083dbc0ad5bb80095bb13dc414a21a3948a5a"
     "33e19",
     "fsrMr9RBY9iHmvJHLc9WNzgwT8DsmQUk3Qhkwkk7MjC6669JpENXBZNTWkdoqg5fuQwknBcgJtphhnYSSxMLDZ9LgZXW8"
     "k272ixQT8BQvXhBHAwKhZVUX3cTSRKru79o1rWbmZuedgB9VAR8AoFrNzovHiiBvkFmHQfBg6k6WcdMMhpguSRrnAaog4"
     "gNw6X2ik"},
    {"1e5758a2ab4aab3a3b2a8202ee925a39245ad27fc6089d263b7dc2a96604793209f4500afeabd1d16c2e9c74ba00f"
     "5eef704551330f4fee12444a2d22a5c00cabb5e7a77445a7d2247a0b51fca96500bfce813b3e3b370df584059d691"
     "6767f7432f2b716e2d0e35ab0b26594ba1c3ecb79a925479d3acabe2e2a510ff4a5f23a9",
     "EhqUP49GkhcauXL8kfJrTuaJ1SCZFwSgido7vfa3bB6NqRoazwt2V7S5qnM57tufHMBghL2XdS5KS2NJ45ur6qbBkPUxs"
     "4dJthAwiNSxxP9Ge9324LJkRg6CLLoE5LUJvf9Cy6dsBMFJ6iGpUgDqv272inXBwSNFq9F4r4WC7kBsvGAU"},
    {"11ee464b216d77bdbaee20b2624ad51780dbff6e8ad5ba179e7a82a90e89aa6541afc2fbfed773c2489ddaa27ac93"
     "a242bcd527e81890e70174f59158d0ba7ed97c20905ba86cad7c538d02caf791b65a8c3117391466accd62f018ed2"
     "f32e410eaf675e9cfcb639445839c8e5fd6c2a55c774ee7fdbd47a94e09a382de214d239bf4ab87de4d4ddaa8ccaf"
     "f925c5b2bc3792dbcc531fcc7521b7697be4fa09ba147f3472e94a9a0a3232d0a47eeb640aa384beff0ddeb108600"
     "21",
     "Krq9Wbrmcq9UdVmXoKhXD2HXmoFZeyGfFgFxVho9DqBKzEHfoffCtoYN9oTrDPG7tudMbXB4WYSiihwRj38gtsUNqRdvH"
     "CJBV1rdoKp9jwrHpCo7Ya3QPbmxpx8ZJrtuWGR7LosCxinJXqdc8QuMFw4HnWhPZBbQLNp43eo3HjswVWypJquzwsYMJh"
     "YKuMfBXyZ1Jv74T3XdnqLSgLajMq9P3PsH7XwaQEG9P58dgLqUVGNh31dLAKcprxmt4uN"},
    {"21b26f8023d34bb7f8e06b903e9ca8bb33d16178881fe267328f2d0473df240ecf992c27320aeb9aed179c76a89bb"
     "06293134cdd299a2ec06b00301831f199101c3db468692b027f1c58e1da9ded06ef5c98d3423c1d339effdda434d8"
     "afd008fa6c5944b1bc9a64ba35ca963a4dbda4e8eefb14661c2b67e8e54983647ed3232a5f1ddbe83ceef9dd3df54"
     "92b00d1380f13de63fa1852840daa462a768d8a4d2c92b50dd6ca104b8a4f47e75d8d3d299be5897543a499e4f0d9"
     "40e30b77aedf9d462d78cad57425f3a54f193726645c3df93a08b15c88e1ab50c70c56ca555d3e2af36ce2afe1b33"
     "7febf80693abe82d9b467c50c3f6bb6e653ed3c",
     "EofN5B6akZYGmn7ghgYuqaHjKr7d6vSsBFKt1Rqt5tkbZMCHA2j1mv2mWP7UTMHqfs6WULGRxZHDcwrW25u4iCc4nBHY9"
     "VSqzf8NefD2QxCnVueQ4RFZ7WhUK8NBZRNoNKao7QxFptnjLxK3heLxaKniE5pTwK6Ft2iHkUVV1x7cinVS6WwvTiJLVG"
     "EPitcM7r6hd1qkyLWTH4vunFcVHuMXmm2G9YS4UkRQyiJDzRDHA23uPK7gLXfKeP26PEPCBQ29FxCM3sWU8F9ekJRoa9v"
     "cJfMeDc7dGqG78gBoAr1QMFeMbcgjfNBjzPHHipK1PEY4zhc9vX1R9EGzqydn7uS3"},
    {"27c25237d8923817a3f29b3b49c66cada8b775054212b18ec7c16d7f83654d10b9245309771d2cd0b9e35e6785091"
     "69b14e27e48930402cf2410",
     "Myda3CdxYU9ugQQF22iuhZ6dVNn23NsKCt2paLY2GLDMy7ohJZoXPGk6QQoCctvMDMizYo9CppcXQBH"},
    {"97da27cc39ae6939dec002de9272ab802d56df0d1b31f908fdc0232fd408a42246e4b2f0bed4a90e225eb08f32504"
     "4a5aae95004abaccb8bac3be780524181830fd76109776dae47fd191e171f30cd0d25b63654006363327355fa2075"
     "1f076361d041478f57e5c6e7b43cbef80d923079cf7f3199f0af0608451ec695739cb7faf9b57193a6c2e9a7a5404"
     "c2855b8e54f8783b561598542418cd1dedc91aa9ef7ddc854d3c40a1cda970a50910f897112c16420612bca",
     "RQe22uhobVBEW5gRwRyjm95WMJVibtFJBErFAWXeKG1RJeCMcxFXPH9WUxh9UcDz9HWyy7AH6Am4YtpoVDmViEhT9FaFa"
     "Li4Vx1j7B5HdiQG5i2YvYTidu5kv3GCt5Rkg1CvC7vMQZSBAy4cuHFrBdCxw34v78cidL5JvH7XQBX99V2N869hi1knWf"
     "3gutA35Ktc92PGWTvcQwShGi38oHXtWSmV8tP3RaeqjvQS6TYrNs2LaSSWM5CbSZ"},
    {"5e8bc24aba6ee6dea38d21ab4cca976c6a5ec6d02441408d11e1e1e94e4c3a9a87d7f6948549def70a66ae2571fcd"
     "e9fa0a9bfd20ad28e9920e9bf64e5bb63ec887554d6d0f8e6ef6cceeff9218b16c20b82a2dc",
     "7k23QbDncCDqThfiCsrjrVfEPHBeabgKCGJTW4mB79yj1qwWUFdz3qQgEKh3vwLRJmFyjaacRt6SCZCUWUZwKUD9ug1CH"
     "CEWD7joTugMFEagHEfmTwR"},
    {"b5a7f4c7c61c78a88dc64ed4663738bd6eadef0f0b662468e9cad4207a4ae8405c68e1211d5ec9dd072ba4b4f3170"
     "ebaa11360bfa108a930d70e6b4bd90513497aed4d8c33f97796180ce7c0933a4a47bc77e9cc699655a8c24a65320a"
     "95e3bc1e69fd85cf721050fcc987cac426c46435d4b9e566c89d007c0acdc4cbf288f8d870d7c59d9b620112a50e1"
     "34bb7dc0e78e14bd0",
     "29JGHxB2vknb9UmPseH78VRoT4v9BBTzZpesMPY22weMYAX7LbKQz6UyG1HPB9CAqzeqVRMREMXGaezEoW3eJvzTUPsxT"
     "YFdiii8kJBRwqXShF9rioXE9ZTPWJaaRdqD6fPPipiwVYAopv63gh7GwEgmkpuxMejrqg6NYkq5Qag8UDMs88SEiYWpXa"
     "1LHwE4aprTZhYD5sV"},
    {"a59a9a0bc0a99eead5cc03bf2006d61cd73b84c9ba99065ada6d9d5abc732a19b342e0016930146b20d3f9fe660f9"
     "1dbfe0601ef7176cafeedc26c4801a3c8e5ea899bf0d79da99270cdec448edc05d189db97c92d1c5cdbe445dc1158"
     "47a50189009e9a4489d56da028c76d5f378686e69b6563451c33154b5190c6763f247b5e84632f3d6792fc369d113"
     "62e69a9c47f842d541615cc31e69ec79464b3f5875e7f8cf76c84f93481a82fd975e2",
     "9AnL21iJC5cpGn19UyBJK2odfxByFkPTFH3NbJmBkg4Fstf5eyK3P26TFZuqTjqhNtcj124BprwkbLMo7TqQQyqkzvHRx"
     "bR6e2M8f4SS1duPAgDU6vFjKrkD4nfkekkjKLgtmrsg58ba2pcipn2MokH37cxvq2gPnT36z7TH3vFn4hzkXTXNsXhpsz"
     "fNrvhXytwSHYD5mXc8umbwvEkn33dGGnXBZJWNX2axm3hytknBgD"}};

class Base58Tests : public ::testing::TestWithParam<TestCase>
{
protected:
};

TEST_P(Base58Tests, CheckEncode)
{
  auto const &test = GetParam();

  ConstByteArray const input{FromHex(test.hex)};
  std::string const    expected{test.base58};
  std::string const    actual{ToBase58(input)};

  EXPECT_EQ(expected, actual);
}

TEST_P(Base58Tests, CheckDecode)
{
  auto const &test = GetParam();

  ConstByteArray const input{test.base58};
  std::string const    expected{test.hex};
  std::string const    actual{ToHex(FromBase58(input))};

  EXPECT_EQ(expected, actual);
}

TEST_P(Base58Tests, CheckDecodeWithTrailingSpacesGoingBeyondBufferEndBoundary)
{
  auto const &test = GetParam();

  ConstByteArray const original_b58_value{test.base58};
  ConstByteArray const characters_beyond_end_boundary{"          "};
  ConstByteArray const concatenated_array{original_b58_value + characters_beyond_end_boundary};
  ConstByteArray const input{concatenated_array.SubArray(0, original_b58_value.size())};
  ASSERT_EQ(original_b58_value, input);
  // This verifies that `input` sub-array points to the same memory as `concatenated_array`
  ASSERT_EQ(concatenated_array.pointer(), input.pointer());
  // The following verifies expected content beyond end boundary of the `input` array and is ought
  // fail if something is wrong (when run with address sanitizer, Valgrind, etc. ...)
  ASSERT_TRUE(
      std::equal(characters_beyond_end_boundary.pointer(),
                 characters_beyond_end_boundary.pointer() + characters_beyond_end_boundary.size(),
                 input.pointer() + input.size()));

  ConstByteArray const expected{test.hex};
  ConstByteArray const actual{ToHex(FromBase58(input))};

  EXPECT_EQ(expected, actual);
}

TEST_F(Base58Tests, CheckDecodeContinuous1GoingBeyondBufferEndBoundary)
{
  ConstByteArray const original_b58_value{"111111"};
  ConstByteArray const characters_beyond_end_boundary{"111111111111"};
  ConstByteArray const concatenated_array{original_b58_value + characters_beyond_end_boundary};
  ConstByteArray const input{concatenated_array.SubArray(0, original_b58_value.size())};
  ASSERT_EQ(original_b58_value, input);
  // This verifies that `input` sub-array points to the same emory as `concatenated_array`
  ASSERT_EQ(concatenated_array.pointer(), input.pointer());
  // The following verifies expected content beyond end boundary of the `input` array and is ought
  // fail if something is wrong (when run with address sanitizer, Valgrind, etc. ...)
  ASSERT_TRUE(
      std::equal(characters_beyond_end_boundary.pointer(),
                 characters_beyond_end_boundary.pointer() + characters_beyond_end_boundary.size(),
                 input.pointer() + input.size()));

  ConstByteArray const expected{"000000000000"};
  ConstByteArray const actual{ToHex(FromBase58(input))};

  EXPECT_EQ(expected, actual);
}

}  // namespace

INSTANTIATE_TEST_CASE_P(ParamBased, Base58Tests, ::testing::ValuesIn(TEST_CASES), );
