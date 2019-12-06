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
persistent balance_state : UInt64;
persistent c2c_balance_state : UInt64;

contract c1_interface
  @action
  function c2c_balance() : UInt64;
endcontract

@action
function c2c_call(contract_address : String)
  use balance_state;
  use c2c_balance_state;

  contract c = c1_interface(contract_address);
  var c2c_bal = c.c2c_balance();
  c2c_balance_state.set(c2c_bal);

  var bal = balance();
  balance_state.set(bal);

  // Test scenario ensures the two have different amounts of funds
  assert(
    c2c_bal != bal,
    "Expected c2c_bal=" + toString(c2c_bal) +
      " and bal=" + toString(bal) + " to be different");
endfunction

@query
function query_balance_state() : UInt64
  use balance_state;

  return balance_state.get();
endfunction

@query
function query_c2c_balance_state() : UInt64
  use c2c_balance_state;

  return c2c_balance_state.get();
endfunction
"""

CONTRACT_TEXT2 = """
@action
function c2c_balance() : UInt64
  return balance();
endfunction
"""


def run(options):
    entity1 = Entity()
    api = LedgerApi(options['host'], options['port'])
    api.sync(api.tokens.wealth(entity1, 100000000))

    contract = Contract(CONTRACT_TEXT, entity1)
    contract2 = Contract(CONTRACT_TEXT2, entity1)

    api.sync(api.contracts.create(entity1, contract, 10000))
    api.sync(api.contracts.create(entity1, contract2, 10000))

    BALANCE1 = 5000
    BALANCE2 = 3000
    assert BALANCE1 != BALANCE2, \
        'Contracts must have different amounts of funds' \
        'to ensure that balance() queries the balance of its respective contract'
    api.sync(api.tokens.transfer(entity1, contract.address, BALANCE1, 200))
    api.sync(api.tokens.transfer(entity1, contract2.address, BALANCE2, 200))

    contract2_name = '{}.{}'.format(
        contract2.digest.to_hex(), contract2.address)
    api.sync(contract.action(api, 'c2c_call',
                             10000, [entity1], contract2_name))

    result_balance1 = contract.query(api, 'query_balance_state')
    assert result_balance1 == BALANCE1, \
        'Expected BALANCE1 {}, found {}'.format(BALANCE1, result_balance1)

    result_balance2 = contract.query(api, 'query_c2c_balance_state')
    assert result_balance2 == BALANCE2, \
        'Expected BALANCE2 {}, found {}'.format(BALANCE2, result_balance2)
