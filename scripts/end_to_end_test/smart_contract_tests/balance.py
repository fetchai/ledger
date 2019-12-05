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
persistent init_test : Int64;
persistent action_test : Int64;
persistent clear_test : Int64;

@init
function init_test()
  use init_test;

  init_test.set(toInt64(balance()));
endfunction

@query
function query_init_test() : Int64
  use init_test;

  return init_test.get(-1i64);
endfunction

@action
function action_test()
  use action_test;

  action_test.set(toInt64(balance()));
endfunction

@query
function query_action_test() : Int64
  use action_test;

  return action_test.get(-1i64);
endfunction

@clear
function clear_test(problem : Int32, new_solution : Int32)
  use clear_test;

  clear_test.set(toInt64(balance()));
endfunction

@query
function query_clear_test() : Int64
  use clear_test;

  return clear_test.get(-1i64);
endfunction

@problem
function createProblem(data : Array<StructuredData>) : Int32
  return 0;
endfunction

@objective
function evaluateWork(problem : Int32, solution : Int32 ) : Int64
  return 1i64;
endfunction

@work
function doWork(problem : Int32, nonce : UInt256) : Int32
  return problem;
endfunction
"""


def submit_synergetic_data(api, contract, data, entity):
    api.sync([api.contracts.submit_data(entity, contract.address, value=value)
              for value in data])

    print('Submitted:', sorted(data))
    print('Waiting...')
    api.wait_for_blocks(10)


def run(options):
    entity1 = Entity()
    api = LedgerApi(options['host'], options['port'])
    api.sync(api.tokens.wealth(entity1, 100000000))
    contract = Contract(CONTRACT_TEXT, entity1)

    api.sync(api.contracts.create(entity1, contract, 10000))

    api.sync(api.tokens.transfer(entity1, contract.address, 1000, 500))

    assert contract.query(api, 'query_init_test') == 0

    api.sync(contract.action(api, 'action_test', 200, [entity1]))
    v = contract.query(api, 'query_action_test')
    assert 800 <= v <= 1000, \
        'Expected query_action_test result to be between 800 and 1000, found {}'.format(
            v)

    submit_synergetic_data(api, contract, [100, 20, 3], entity1)
    v = contract.query(api, 'query_clear_test')
    assert 800 <= v <= 1000, \
        'Expected query_clear_test result to be between 800 and 1000, found {}'.format(
            v)

    # Provide the contract with funds
    api.sync(api.tokens.transfer(entity1, contract.address, 1234, 200))

    api.sync(contract.action(api, 'action_test', 200, [entity1]))
    v = contract.query(api, 'query_action_test')
    assert 2000 < v < 2234, \
        'Expected query_action_test result to be between 2000 and 2234, found {}'.format(
            v)

    submit_synergetic_data(api, contract, [100, 20, 3], entity1)
    v = contract.query(api, 'query_clear_test')
    assert 2000 < v < 2234, \
        'Expected query_clear_test result to be between 2000 and 2234, found {}'.format(
            v)
