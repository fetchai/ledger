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

TERMINAL_CONTRACT_TEXT = """
@action
function c2c_call()
endfunction
"""


def run(options, benefactor):
    entity1 = Entity()

    # create the APIs
    api = LedgerApi(options['host'], options['port'])

    api.sync(api.tokens.transfer(benefactor, entity1, 1000000, 1000))

    contracts = []
    for i in range(20):
        contracts += [Contract(CONTRACT_TEXT, entity1)]
    contract_terminal = Contract(TERMINAL_CONTRACT_TEXT, entity1)

    for contract in contracts:
        api.sync(api.contracts.create(entity1, contract, 2000))
    api.sync(api.contracts.create(entity1, contract_terminal, 2000))

    for i in range(len(contracts) - 1):
        api.sync(contracts[i].action(api, 'set_callee',
                                     400, [entity1], str(contracts[i + 1].address)))
    api.sync(contracts[-1].action(api, 'set_callee',
                                  400, [entity1], str(contract_terminal.address)))

    try:
        api.sync(contracts[0].action(api, 'c2c_call', 400, [entity1]))
        assert False, \
            'Expected transaction to fail'
    except RuntimeError:
        pass
