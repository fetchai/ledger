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
persistent block_number_state : Int64;
// persistent contract_address_state : Address;

contract c1_interface
  // @action
  // function contract_address() : Int64;
  @action
  function block_number() : Int64;
endcontract

@action
function c2c_call(contract_name : String)
  use block_number_state;
  // use contract_address_state;

  contract c = c1_interface(contract_name);

  var c2c_block_number = c.block_number();
  block_number_state.set(c2c_block_number);

  // c.contract_address();
  // get contract address from query when supported: var c2c_addr =
  // contract_address_state.set(c2c_addr);

  var context = getContext();
  var block = context.block();
  var transaction = context.transaction();
  var block_number = block.blockNumber();
  // var addr = transaction.contractAddress();

  // assert(c2c_addr == addr);
  assert(c2c_block_number == toInt64(block_number));
endfunction

@query
function query_block_number_state() : Int64
  use block_number_state;

  return block_number_state.get();
endfunction

// @query
// function query_contract_address_state() : Address
//   use contract_address_state;
//
//   return contract_address_state.get();
// endfunction
"""

CONTRACT_TEXT2 = """
// @action // convert to action-query pair
// function contract_address() : Address
//   var context = getContext();
//   var transaction = context.transaction();
//   var addr = transaction.contractAddress();
//
//   return addr;
// endfunction

@action
function block_number() : Int64
  var context = getContext();
  var block = context.block();
  var block_number = block.blockNumber();

  return toInt64(block_number);
endfunction
"""


def run(options):
    entity1 = Entity()

    # create the APIs
    api = LedgerApi(options['host'], options['port'])

    api.sync(api.tokens.wealth(entity1, 1000000))

    contract = Contract(CONTRACT_TEXT, entity1)
    contract2 = Contract(CONTRACT_TEXT2, entity1)

    api.sync(api.contracts.create(entity1, contract, 20000))
    api.sync(api.contracts.create(entity1, contract2, 20000))

    current_block = api.tokens._current_block_number()
    contract2_name = "{}.{}".format(
        contract2.digest.to_hex(), contract2.address)
    api.sync(contract.action(api, 'c2c_call',
                             40000, [entity1], contract2_name))

    time.sleep(2)

    later_block = contract.query(api, 'query_block_number_state')
    assert later_block > current_block, \
        'Expected number larger than {}, found {}'.format(
            current_block, later_block)

    # result = contract.query(api, 'query_contract_address_state')
    # assert len(str(result)) > 0, \
    #     'Expected non-empty address'
    # assert str(result) == str(contract.address), \
    #     'Expected query_contract_address_state to return calling contract\'s ' \
    #     'address ({}), but instead got {}'.format(contract.address, result)
