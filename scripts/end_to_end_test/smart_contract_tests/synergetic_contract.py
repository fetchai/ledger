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

import time

from fetchai.ledger.api import LedgerApi
from fetchai.ledger.contract import Contract
from fetchai.ledger.crypto import Entity

CONTRACT_TEXT = """
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
//  return abs(toInt64(problem) - toInt64(solution));
endfunction

@work
function doWork(problem : Int32, nonce : UInt256) :  Int32
  return problem;//nonce.toInt32();
endfunction

@clear
function applyWork(problem : Int32, solution : Int32)
  var result = State<Int32>("solution");
  result.set(solution);
endfunction

@query
function query_result() : Int32
  var result = State<Int32>("solution");
  return result.get(-1);
endfunction
"""


def submit_synergetic_data(api, contract, data, entity):
    api.sync([api.contracts.submit_data(entity, contract.digest, value=value)
              for value in data])

    print('Submitted:', sorted(data))
    print('Waiting...')
    time.sleep(10)

    result = contract.query(api, 'query_result')
    print('Result:', result)
    assert result == sum(data)


def run(options):
    entity1 = Entity()

    # build the ledger API
    api = LedgerApi(options['host'], options['port'])

    # create wealth so that we have the funds to be able to create contracts on the network
    api.sync(api.tokens.wealth(entity1, 100000000))

    # ???remove need for type param
    contract = Contract(CONTRACT_TEXT)

    # deploy the contract to the network
    api.sync(api.contracts.create(entity1, contract, 2000))

    submit_synergetic_data(api, contract, [100, 20, 3], entity1)
    submit_synergetic_data(api, contract, [11, 22, 33], entity1)

# ???submit solutions from more than one pubkey
# ???check action and init
# ???check mining fees
# ???check what nonces are being submitted
# ???inherently unstable test-different results if submission caught between epochs
# ???sharded state
