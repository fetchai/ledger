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

#include "crypto/ecdsa.hpp"

#include <cstdlib>
#include <fstream>

namespace fetch {

using fetch::crypto::Prover;
using fetch::crypto::ECDSASigner;

using SignerPtr = std::shared_ptr<ECDSASigner>;
using ProverPtr = std::shared_ptr<Prover>;

char const *LOGGING_NAME         = "KeyGenerator";
char const *DEFAULT_KEY_FILENAME = "p2p.key";

/**
 * Lookup the key path for the key
 *
 * @return The path to the key file
 */
char const *GetKeyPath()
{
  char const *path = std::getenv("CONSTELLATION_KEY_PATH");

  if (path == nullptr)
  {
    path = DEFAULT_KEY_FILENAME;
  }

  return path;
}

/**
 * Attempt to load a previous key key file
 *
 * @return The shared pointer to the new key
 */
ProverPtr GenerateP2PKey()
{
  char const *key_path = GetKeyPath();

  SignerPtr certificate        = std::make_shared<ECDSASigner>();
  bool      certificate_loaded = false;

  // Step 1. Attempt to load the existing key
  {
    std::ifstream input_file(key_path, std::ios::in | std::ios::binary);

    if (input_file.is_open())
    {
      fetch::byte_array::ByteArray private_key_data;
      private_key_data.Resize(ECDSASigner::PrivateKey::ecdsa_curve_type::privateKeySize);

      // attempt to read in the private key
      input_file.read(private_key_data.char_pointer(),
                      static_cast<std::streamsize>(private_key_data.size()));

      if (!(input_file.fail() || input_file.eof()))
      {
        certificate->Load(private_key_data);
        certificate_loaded = true;
      }
    }
  }

  // Generate a key if the load failed
  if (!certificate_loaded)
  {
    certificate->GenerateKeys();

    std::ofstream output_file(key_path, std::ios::out | std::ios::binary);

    if (output_file.is_open())
    {
      auto const private_key_data = certificate->private_key();

      output_file.write(private_key_data.char_pointer(),
                        static_cast<std::streamsize>(private_key_data.size()));
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to save P2P key");
    }
  }

  return certificate;
}

}  // namespace fetch
