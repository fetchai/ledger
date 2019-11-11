# ------------------------------------------------------------------------------
#
#   Copyright 2018-2019 Fetch.AI Limited
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# ------------------------------------------------------------------------------

from fetchai.ledger.api import LedgerApi
from fetchai.ledger.contract import Contract
from fetchai.ledger.crypto import Entity, Address

CONTRACT_TEXT = """
persistent owner_address : Address;

@init
function set_up(owner : Address)
  use owner_address;

  owner_address.set(owner);
endfunction

@query
function query_owner_address() : String
  use owner_address;

  return toString(owner_address.get());
endfunction
"""


def run(options):
    entity1 = Entity()

    # build the ledger API
    api = LedgerApi(options['host'], options['port'])

    # create wealth so that we have the funds to be able to create contracts on the network
    api.sync(api.tokens.wealth(entity1, 100000))

    contract = Contract(CONTRACT_TEXT, entity1)

    # deploy the contract to the network
    status = api.sync(api.contracts.create(entity1, contract, 2000))[0]

    saved_address = contract.query(api, 'query_owner_address')

    assert str(Address(entity1)) == saved_address, \
        'Expected owner address {} but found {}'.format(
            Address(entity1), saved_address)
