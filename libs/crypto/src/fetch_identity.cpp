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

#include "crypto/fetch_identity.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "crypto/fnv.hpp"

#include <unordered_set>

using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace crypto {
namespace {

const std::unordered_set<ConstByteArray> FETCH_IDENTITIES{
  "MnrRHdvCkdZodEwM855vemS5V3p2hiWmcSQ8JEzD4ZjPdsYtB",
  "2bBYqHp5uK8fQgTqeBP3B3rogHQPYiC6wZcnBP2WVocsuiMgg9",
  "LAxzfxivjQSgUhv6ju6Q6tMjc8G996HGBGtG482jZwH3P77MF",
  "2oTySoewvvu85Y5oBaY6GaHUi1A2jw9RyLdzPyeXHsBKBDpUe8",
  "yqjE1nCKLYRS3DUeFPmumR4sjQJFRYvKU2KQhdUFKQgkVUr6y",
  "7pwsmX7TGG3XRrDeTFDkBNcfKJGBeG9Rq85j6vcKEUTLZxzyu",
  "2ZfLWYyaHv66qsSeb9g1ki8ShH6nrFT3TPbeL4FUYoLdwq8qEH",
  "2SmuQo82jxVoi8qY5NitcRQVWzi4g9CyXpR1VJrDjjq9nrQwWN",
  "MfiLMVPWsCJXEuAT2j8z9gKoWZ5RumEYr97EJUK2XPyJc3xMu",
  "3yxLj2h1AsBpfacUjn1Pph3jJYi83oZNKbKMT2qVvn4M3Xja7",
  "vdYrsGTxk5pWLuvgFPuv4tCCJ7c7qaQrmiNsraqS7Gpwd58ij",
  "BDr2FmQK1yNpEY9RjCiauNZXcJkm2EjYtLta94N5cDyGVLWaf",
  "V4aL9eeLj3r42tVZztJq3mDLWJK3jPPNVc2bfAizmXozKhoru",
  "2Y7CS65GVgntrW4BohCTPpPCGExEFVzfEw1DYZJstv1PEVyjRE",
  "vtmD29suwXx63y6WhCu52oBGALcY59fcZmvqFh8MCETyc5k2e",
  "2cxb5h2HMer5uBDAs7X4EpXuBZSbfX5UUvhGwBHQMB3ZtCkMsm",
  "2KyS2GfJ9iYxGHVtHxv2pyreW5XZXgAxHH5jXYkSruYTj8KYcR",
  "Lz2yNwzXSDDMCEU3Mt8ARfK8aqRiX35d6XWGhdhWYiBQymtvB",
  "H381FKAkk9aVCF4SWcocDZYe9Wgs6dGxvFERMRkGYG5yt6dff",
  "2m5bhKu4SrRqJpbeKk3Th3gc5gedYDPfT7BELhLnpvTugjmS4r"
};

}

bool IsFetchIdentity(byte_array::ConstByteArray const &identity)
{
  return FETCH_IDENTITIES.find(identity) != FETCH_IDENTITIES.end();
}

}  // namespace crypto
}  // namespace fetch