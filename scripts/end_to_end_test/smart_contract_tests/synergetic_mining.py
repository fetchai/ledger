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
persistent clear_balance_state : UInt64;

@init
function init_test()
  var state = State<Int32>("init_test");
  state.set(123);
endfunction

@query
function query_init_test() : Int32
  var state = State<Int32>("init_test");
  return state.get(-1);
endfunction

@action
function action_test()
  var state = State<Int32>("action_test");
  state.set(456);
endfunction

@query
function query_action_test() : Int32
  var state = State<Int32>("action_test");
  return state.get(-1);
endfunction

@problem
function createProblem(data : Array<StructuredData>) : Int32
  var value = 0;
  for (i in 0:data.count())
    value += data[i].getInt32("value");
  endfor
  return value;
endfunction

@objective
function evaluateWork(problem : Int32, solution : Int32 ) : Int64
  return abs(toInt64(problem));
endfunction

@work
function doWork(problem : Int32, nonce : UInt256) :  Int32
  return problem;
endfunction

@clear
function applyWork(problem : Int32, solution : Int32)
  use clear_balance_state;

  var result = State<Int32>("solution");
  result.set(result.get(0) + solution);
  clear_balance_state.set(balance());
endfunction

@query
function query_result() : Int32
  var result = State<Int32>("solution");
  return result.get(-1);
endfunction

//???move to dedicated @clear test?
@query
function query_clear_balance_state() : UInt64
  use clear_balance_state;

  return clear_balance_state.get(9876u64);
endfunction
"""


def submit_synergetic_data(api, contract, data, entity):
    api.sync([api.contracts.submit_data(entity, contract.digest, contract.address, value=value)
              for value in data])

    print('Submitted:', sorted(data))
    print('Waiting...')
    api.wait_for_blocks(10)

    result = contract.query(api, 'query_result')
    print('Result:', result)
    assert result == sum(data), \
        'Expected {}, found {}'.format(sum(data), result)


def run(options):
    entity1 = Entity()

    # build the ledger API
    api = LedgerApi(options['host'], options['port'])

    # create wealth so that we have the funds to be able to create contracts on the network
    print('Create wealth...')
    api.sync(api.tokens.wealth(entity1, 100000000))

    contract = Contract(CONTRACT_TEXT, entity1)

    # deploy the contract to the network
    print('Create contract...')
    api.sync(api.contracts.create(entity1, contract, 10000))

    submit_synergetic_data(api, contract, [100, 20, 3], entity1)

    print('Query init state...')
    init_result = contract.query(api, 'query_init_test')
    print('Init state: ', init_result)
    assert init_result == 123

    print('Execute action...')
    api.sync(contract.action(api, 'action_test', 10000, [entity1]))

    print('Query action state...')
    action_result = contract.query(api, 'query_action_test')
    print('Action state: ', action_result)
    assert action_result == 456

    print('Query clear balance state...')
    clear_balance_result = contract.query(api, 'query_clear_balance_state')
    print('Clear balance result: ', clear_balance_result)
    assert clear_balance_result == 0
