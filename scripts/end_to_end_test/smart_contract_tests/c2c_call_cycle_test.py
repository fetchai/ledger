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
persistent callee_address : String;

contract c1_interface
  @action
  function c2c_call();
endcontract

@action
function set_callee(addr : String)
  use callee_address;

  callee_address.set(addr);
endfunction

@action
function c2c_call()
  use callee_address;

  contract c = c1_interface(callee_address.get());
  c.c2c_call();
endfunction
"""


def run(options):
    entity1 = Entity()

    # create the APIs
    api = LedgerApi(options['host'], options['port'])

    api.sync(api.tokens.wealth(entity1, 1000000))

    contract0 = Contract(CONTRACT_TEXT, entity1)
    contract1 = Contract(CONTRACT_TEXT, entity1)
    contract2 = Contract(CONTRACT_TEXT, entity1)
    contract3 = Contract(CONTRACT_TEXT, entity1)

    api.sync(api.contracts.create(entity1, contract0, 2000))
    api.sync(api.contracts.create(entity1, contract1, 2000))
    api.sync(api.contracts.create(entity1, contract2, 2000))
    api.sync(api.contracts.create(entity1, contract3, 2000))

    contract_name0 = "{}.{}".format(
        contract0.digest.to_hex(), contract0.address)
    contract_name1 = "{}.{}".format(
        contract1.digest.to_hex(), contract1.address)
    contract_name2 = "{}.{}".format(
        contract2.digest.to_hex(), contract2.address)
    contract_name3 = "{}.{}".format(
        contract3.digest.to_hex(), contract3.address)

    api.sync(contract0.action(api, 'set_callee',
                              400, [entity1], contract_name1))
    api.sync(contract1.action(api, 'set_callee',
                              400, [entity1], contract_name2))
    api.sync(contract2.action(api, 'set_callee',
                              400, [entity1], contract_name3))
    api.sync(contract3.action(api, 'set_callee',
                              400, [entity1], contract_name0))

    try:
        api.sync(contract0.action(api, 'c2c_call',
                                  400, [entity1]))
        assert False, \
            'Expected transaction to fail'
    except RuntimeError:
        pass
