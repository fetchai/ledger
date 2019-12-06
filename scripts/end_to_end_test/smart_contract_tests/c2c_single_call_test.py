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
from fetchai.ledger.crypto import Entity

CONTRACT_TEXT = """
persistent x : UInt8;

contract c1_interface
  @action
  function eleven() : UInt8;
endcontract

@action
function c2c_call(contract_name : String)
  use x;

  contract c = c1_interface(contract_name);
  x.set(c.eleven());
endfunction

@query
function query_eleven() : UInt8
  use x;

  return x.get();
endfunction
"""

CONTRACT_TEXT2 = """
@action
function eleven() : UInt8
  return 11u8;
endfunction
"""


def run(options):
    entity1 = Entity()

    # create the APIs
    api = LedgerApi(options['host'], options['port'])

    api.sync(api.tokens.wealth(entity1, 1000000))

    contract = Contract(CONTRACT_TEXT, entity1)
    contract2 = Contract(CONTRACT_TEXT2, entity1)

    api.sync(api.contracts.create(entity1, contract, 2000))
    api.sync(api.contracts.create(entity1, contract2, 2000))

    contract_name = "{}.{}".format(
        contract2.digest.to_hex(), contract2.address)
    api.sync(contract.action(api, 'c2c_call', 400, [entity1], contract_name))

    result = contract.query(api, 'query_eleven')
    assert result == 11, \
        'Expected 11, found {}'.format(result)