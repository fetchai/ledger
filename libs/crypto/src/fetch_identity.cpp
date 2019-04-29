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
#include "core/byte_array/decoders.hpp"
#include "crypto/identity.hpp"

#include <unordered_set>

using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace crypto {
namespace {

const std::unordered_set<ConstByteArray> FETCH_IDENTITIES{
    FromBase64(
        "FOLkhxXQM09DJh/SVq+V7dy7QjhFzp4RWQDopWjbCV03Lc0agxMGY+btv8y6o5Dlk0Dw7Nhd/rvt8lc9gZsoXw=="),
    FromBase64(
        "CL3zb8U2KYP+OWmn9wZuHuUIXciSNtTZPjmgB75OHRk8hxgCt+G+sOj1dXVUqzsMcNYAJwV/tScKu8ej1yAuDw=="),
    FromBase64(
        "PdvmYkh9Vj9aLW/g4bcaWIKIPyv3o8EuvbnNmGROSy+gEVboTmPGERSnee6HcZ/Xf9KyRtaHf7nQF7lwi0mQYA=="),
    FromBase64(
        "3cshquIkEPui6wYtlkSzkVNR5tBFy3bsILnBdqOjgxdtDCSfciQlMgd5Xzv+aMySljjtCw1NZbRwTeXHAD218A=="),
    FromBase64(
        "I5wWf+ydbwLO/2NxnbUpqQiM3Qi/d3a8mLq5BmvEXtoff51aukjK8gd2RSNWlJZTHdVoghODgjQMxcx7Gp4Uig=="),
    FromBase64(
        "+x4vT0v5nMmZQkBz70hl/GbT/LdSfouPL6PU3neH+VAmcPSEgFACfIpvZuQHyub0j4t9NDSc/MGVoEG2v7yCzg=="),
    FromBase64(
        "H7zlVQc7tKhzz/3aQhF1n5X/+kOhn5ExOaZwtKv+ZNnqyD0huB8eD7tOyTuLtQSsnmdXfxxfmPrK9MKBoj4g2g=="),
    FromBase64(
        "IRNRUmdlXWP4N7e9ihCgBwTM5pPK/WvxWGgN/c+9rH/JAi/Ox2X0pFcY58/yDrjWPU8OIusFDTKy4trRyrKflA=="),
    FromBase64(
        "FXGPdUfZsfCSwAb4e/jnA+ZhLm0Gi5IXVCP0xCQSkRi0L1+3WbMJ4v0ztteZ2UGgNV0WZ1T6vG4DrdvvKd9/Ow=="),
    FromBase64(
        "TYLRiGI2pGp+RNAQd4ytbxFr6tgbCQO/GYw8bh64F5C7pJs7PBezlXElIfSlPzsda7HIk+CaU5/zbrobrFfWdw=="),
    FromBase64(
        "PT15DmVHc9EcfYLERr+pDn4bId9zNxe9PopkZSMBBUc2SoKT6zMUb+DIRrmJXwnG8j6SgxcO8EaITYmQ6qYL8Q=="),
    FromBase64(
        "dpZc6WRTj7pG3slhKDZmeCKbRF76wOEv+4QWaSQtBV5t/mNzvfTZKfuLj8+pZ5HCkHMhjgYGidy5gBmghJtiFg=="),
    FromBase64(
        "+NcPQpP6/92V5U6pnwtG696dwQ7FZqn/n44Av2HkisCFMgDMJuFUeC2LlZ27hO/Z+RH5kHmvRxWiqSrE9U3YUA=="),
    FromBase64(
        "X3vI2Nzrca5HJlpvbd9+VlQDChsZ3LXKK4Ip2KeMxiOm22UtmW6M0kb6efV+IX8+/dzqVgZpmjpsfmCOodCUxg=="),
    FromBase64(
        "iwldt7ee8ChazUQVRgfHF0TB0mShLuNnG7dBnZQGrTCBj8tsq9JjtO0EsUEUy/gsUYH6xitcsmRr2wxsFvtEFQ=="),
    FromBase64(
        "huQgsBCpf8Bty0A7o8d3FDU/BPsJk2EBGp4Lutnfy3mkbZn+AEVm+OS1I9RoYZcsSgcns4yUr+D2blYls39hUA=="),
    FromBase64(
        "3nZHpx422cwXb2jlo4a5OuBM+wyPg3v20XgF47XxwrMl11W+Vb6clBLjE6mg8t6Y8wPtkt0u1wZ4SHG7PEAHCw=="),
    FromBase64(
        "7tn6JC02L/ix29bwQdHUjvw48tMWGFO6Stz8kyJvabD+bX6ue9qKn5XaWou74UoOVJ2A9san9QUz5rDtvR6KLw=="),
    FromBase64(
        "9YtlC6WOnHmkK63Kw5vR3btpZ83cyUHyT6njyyHEcfHjXgi+dqzpQLeW63cStmDGsRK4qPLoUoXkOsW2OMGg6A=="),
    FromBase64(
        "ub/"
        "bzao13B7iD5LaI33wJAtrxs4FqhPrb90jqiMwyRgT8wAncF796U2v2Fo2Euza+jpc1yLeC8MONmQjFnfdbw==")};
}

bool IsFetchIdentity(Identity const &identity)
{
  return IsFetchIdentity(identity.identifier());
}

bool IsFetchIdentity(byte_array::ConstByteArray const &identity)
{
  return FETCH_IDENTITIES.find(identity) != FETCH_IDENTITIES.end();
}

}  // namespace crypto
}  // namespace fetch