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
persistent magic_first : String;
persistent magic_last : String;
persistent whole_magic_word : String;

@init
function initialise(owner : Address)
  use magic_first;
  use magic_last;

  magic_first.set('ab');
  magic_last.set('bra');
endfunction

contract c1_interface
  @action
  function get_magic_word(contract_name : String) : String;
endcontract

@action
function nested_c2c_call(contract_name : String, nested_contract_name : String)
  use whole_magic_word;
  use magic_first;
  use magic_last;

  var first = magic_first.get();

  contract c = c1_interface(contract_name);
  var middle = c.get_magic_word(nested_contract_name);

  var last = magic_last.get();

  whole_magic_word.set(first + middle + last);
endfunction

@query
function query_magic_word() : String
  use whole_magic_word;

  return whole_magic_word.get();
endfunction
"""

CONTRACT_TEXT2 = """
persistent magic_second : String;
persistent magic_fourth : String;

@init
function initialise(owner : Address)
  use magic_second;
  use magic_fourth;

  magic_second.set('ra');
  magic_fourth.set('da');
endfunction

contract sorcery
  @action
  function cast_spell() : String;
endcontract

@action
function get_magic_word(contract_name : String) : String
  use magic_second;
  use magic_fourth;

  var second = magic_second.get();

  contract s = sorcery(contract_name);
  var third = s.cast_spell();

  var fourth = magic_fourth.get();

  return second + third + fourth;
endfunction
"""


CONTRACT_TEXT3 = """
persistent magic_third : String;

@init
function initialise(owner : Address)
  use magic_third;

  magic_third.set('ca');
endfunction

@action
function cast_spell() : String
  use magic_third;

  return magic_third.get();
endfunction
"""


def run(options):
    entity1 = Entity()

    # create the APIs
    api = LedgerApi(options['host'], options['port'])

    api.sync(api.tokens.wealth(entity1, 1000000))

    contract = Contract(CONTRACT_TEXT, entity1)
    contract2 = Contract(CONTRACT_TEXT2, entity1)
    contract3 = Contract(CONTRACT_TEXT3, entity1)

    api.sync(api.contracts.create(entity1, contract, 2000))
    api.sync(api.contracts.create(entity1, contract2, 2000))
    api.sync(api.contracts.create(entity1, contract3, 2000))

    contract_name = "{}.{}".format(
        contract2.digest.to_hex(), contract2.address)
    nested_contract_name = "{}.{}".format(
        contract3.digest.to_hex(), contract3.address)

    api.sync(contract.action(api, 'nested_c2c_call', 400, [
        entity1], contract_name, nested_contract_name))

    result = contract.query(api, 'query_magic_word')
    assert result == 'abracadabra', \
        'Expected abracadabra, found {}'.format(result)
