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
persistent solution : Int32;
persistent init_test : Int32;
persistent action_test : Int32;

@init
function init_test()
  use init_test;

  init_test.set(123);
endfunction

@query
function query_init_test() : Int32
  use init_test;

  return init_test.get(-1);
endfunction

@action
function action_test()
  use action_test;

  action_test.set(456);
endfunction

@query
function query_action_test() : Int32
  use action_test;

  return action_test.get(-1);
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
function applyWork(problem : Int32, new_solution : Int32)
  use solution;

  solution.set(solution.get(0) + new_solution);
endfunction

@query
function query_result() : Int32
  use solution;

  return solution.get(-1);
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


def run(options, benefactor):
    entity1 = Entity()

    # build the ledger API
    api = LedgerApi(options['host'], options['port'])

    # create funds so that we have the funds to be able to create contracts on the network
    # TODO(HUT): fix this.
    print('Create funds...')
    api.sync(api.tokens.transfer(benefactor, entity1, 100000000, 1000))

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
