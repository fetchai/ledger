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
persistent balance_state : UInt64;

@action
function set_balance_state()
  use balance_state;

  balance_state.set(balance());
endfunction

@query
function query_balance_state() : UInt64
  use balance_state;

  return balance_state.get(567u64);
endfunction
"""

TRANSFER_CONTRACT_TEXT = """
@action
function release_funds(target : Address, amount : UInt64)
  releaseFunds(target, amount);
endfunction
"""


def balance_within_range(actual, expected, fees=100):
    return actual >= expected - fees


def setup(api):
    entity1 = Entity()

    api.sync(api.tokens.wealth(entity1, 100000))

    contract1 = Contract(TRANSFER_CONTRACT_TEXT, entity1)
    contract2 = Contract(CONTRACT_TEXT, entity1)

    api.sync(api.contracts.create(entity1, contract1, 2000))
    api.sync(api.contracts.create(entity1, contract2, 2000))

    direct_balance1 = api.tokens.balance(contract1.address)
    assert direct_balance1 == 0, \
        'Expected initial directly-queried balance to be 0, found {}'.format(
            direct_balance1)

    print('Transfer from owner {} to contract {}'.format(
        Address(entity1), contract1.address))
    api.sync(api.tokens.transfer(entity1, contract1.address, 12345, 100))

    direct_balance2 = api.tokens.balance(contract1.address)
    assert direct_balance2 == 12345, \
        'Expected directly-queried balance to be 12345, found {}'.format(
            direct_balance2)

    return entity1, contract1, contract2


def run(options):
    api = LedgerApi(options['host'], options['port'])
    entity1, contract1, contract2 = setup(api)

    owner_balance_before_transfer = api.tokens.balance(Address(entity1))
    contract1_balance_before_transfer = api.tokens.balance(contract1.address)
    contract2_balance_before_transfer = api.tokens.balance(contract2.address)

    action_args = (contract2.address, 2345)
    api.sync(contract1.action(api, 'release_funds',
                              100, [entity1], *action_args))

    owner_balance_after_transfer = api.tokens.balance(Address(entity1))
    contract1_balance_after_transfer = api.tokens.balance(contract1.address)
    contract2_balance_after_transfer = api.tokens.balance(contract2.address)

    assert balance_within_range(owner_balance_after_transfer, owner_balance_before_transfer), \
        'Expected owner balance to remain roughly unchanged'
    assert contract1_balance_after_transfer == contract1_balance_before_transfer - 2345
    assert contract2_balance_after_transfer == contract2_balance_before_transfer + 2345
