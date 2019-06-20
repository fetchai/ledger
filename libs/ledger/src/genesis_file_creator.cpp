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

#include "ledger/genesis_loading/genesis_file_creator.hpp"

namespace fetch
{

  GenesisFileCreator::GenesisFileCreator(MainChain &chain, StorageUnitInterface &storage_unit)
    : chain_{chain}
    , storage_unit_{storage_unit}
  {
    FETCH_UNUSED(chain_);
    FETCH_UNUSED(storage_unit_);
  }

  void GenesisFileCreator::CreateFile(std::string const &name)
  {
    FETCH_UNUSED(name);
  }

}  // namespace fetch

