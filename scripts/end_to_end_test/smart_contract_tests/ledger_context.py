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
persistent action_block_number_state : UInt64;
persistent init_block_number_state : UInt64;

@init
function set_init_block_number_state(owner : Address) : Int64
  use init_block_number_state;

  var context = getContext();
  var block = context.block();
  var block_number = block.blockNumber();

  init_block_number_state.set(block_number);

  return toInt64(block_number);
endfunction

@query
function get_init_block_number_state() : UInt64
  use init_block_number_state;

  return init_block_number_state.get(0u64);
endfunction

@action
function set_block_number_state() : Int64
  use action_block_number_state;

  var context = getContext();
  var block = context.block();
  var block_number = block.blockNumber();

  action_block_number_state.set(block_number);

  return toInt64(block_number);
endfunction

@query
function query_block_number_state() : UInt64
  use action_block_number_state;

  return action_block_number_state.get(0u64);
endfunction
"""


def run(options, benefactor):
    entity1 = Entity()

    # build the ledger API
    api = LedgerApi(options['host'], options['port'])

    # create funds so that we have the funds to be able to create contracts on the network
    api.sync(api.tokens.transfer(benefactor, entity1, 100000, 1000))

    contract = Contract(CONTRACT_TEXT, entity1)

    # deploy the contract to the network
    status = api.sync(api.contracts.create(entity1, contract, 2000))[0]

    api.sync(api.tokens.transfer(entity1, contract.address, 10000, 500))

    block_number = status.exit_code
    v = contract.query(api, 'get_init_block_number_state')
    assert block_number > 0
    assert block_number == v, \
        'Returned block number by the contract ({}) is not what expected ({})'.format(
            v, block_number)

    api.sync(contract.action(api, 'set_block_number_state', 400, [entity1]))

    assert block_number <= contract.query(api, 'query_block_number_state')
