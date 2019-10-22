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
persistent init_balance_state : UInt64;

@init
function set_initial_balance_state() : Int64
  use init_balance_state;
  init_balance_state.set(888u64);
  // ??? init_balance_state.set(balance());

  return 777i64; // ??? toInt64(balance());
endfunction

@query
function get_initial_balance_state() : UInt64
  use init_balance_state;
  return init_balance_state.get(999u64);
endfunction

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

@query
function query_balance() : UInt64
  return balance();
endfunction
"""


def run(options):
    entity1 = Entity()

    # build the ledger API
    api = LedgerApi(options['host'], options['port'])

    api.sync(api.tokens.wealth(entity1, 1000000))

    contract1 = Contract(CONTRACT_TEXT, entity1)

    api.sync(api.contracts.create(entity1, contract1, 2000))
# ???
    # query_init_balance = contract1.query(api, 'get_initial_balance_state')
    # assert query_init_balance == 0, \
    #     'Expected returned balance to be 0, found {}'.format(
    #         query_init_balance)

    direct_balance1 = api.tokens.balance(contract1.address)
    assert direct_balance1 == 0, \
        'Expected initial directly-queried balance to be 0, found {}'.format(
            direct_balance1)

    api.sync(contract1.action(api, 'set_balance_state', 100, [entity1]))
    query_balance1 = contract1.query(api, 'query_balance_state')
    assert query_balance1 == 0, \
        'Expected initial returned balance to be 0, found {}'.format(
            query_balance1)

    print('Transfer from owner {} to contract {}'.format(
        Address(entity1), contract1.address))
    api.sync(api.tokens.transfer(entity1, contract1.address, 12345, 100))

    direct_balance2 = api.tokens.balance(contract1.address)
    assert direct_balance2 == 12345, \
        'Expected directly-queried balance to be 12345, found {}'.format(
            direct_balance2)

    api.sync(contract1.action(api, 'set_balance_state', 100, [entity1]))
    query_balance2 = contract1.query(api, 'query_balance_state')
    assert query_balance2 == 12345, \
        'Expected returned balance to be 12345, found {}'.format(
            query_balance2)

    query_balance3 = contract1.query(api, 'query_balance')
    assert query_balance3 == 12345, \
        'Expected returned balance to be 12345, found {}'.format(
            query_balance3)


# ???
if __name__ == "__main__":
    run({"host": "127.0.0.1", "port": 8000})
