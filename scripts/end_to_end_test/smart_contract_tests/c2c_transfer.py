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

@action
function transfer_funds(target : Address, amount : UInt64)
  transfer(target, amount);
endfunction
"""

TRANSFER_CONTRACT_TEXT = """
@action
function transfer_funds(target : Address, amount : UInt64)
  transfer(target, amount);
endfunction
"""

MASTER_CONTRACT_TEXT = """
contract c1_interface
  @action
  function transfer_funds(target : Address, amount : UInt64);
endcontract

@action
function execute_transfer(contract_address : String, target : Address, amount : UInt64)
  contract c = c1_interface(contract_address);
  c.transfer_funds(target, amount);
endfunction
"""


def balance_within_range(actual, expected, fees=150):
    return actual >= expected - fees


def setup(api):
    entity1 = Entity()

    api.sync(api.tokens.transfer(benefactor, entity1, 100000, 1000))

    contract1 = Contract(TRANSFER_CONTRACT_TEXT, entity1)
    contract2 = Contract(CONTRACT_TEXT, entity1)
    master_contract = Contract(MASTER_CONTRACT_TEXT, entity1)

    initial_owner_balance = api.tokens.balance(Address(entity1))
    assert initial_owner_balance == 100000, \
        'Expected initial directly-queried balance to be 0, found {}'.format(
            100000, initial_owner_balance)

    api.sync(api.contracts.create(entity1, contract1, 2000))
    api.sync(api.contracts.create(entity1, contract2, 2000))
    api.sync(api.contracts.create(entity1, master_contract, 2000))

    return entity1, contract1, contract2, master_contract


def transfer_and_verify_balances(api, entity, address_to, amount):
    from_balance_before = api.tokens.balance(Address(entity))
    to_balance_before = api.tokens.balance(address_to)

    assert from_balance_before > amount, \
        'Insufficient funds'

    print('Transfer from {} to {}'.format(Address(entity), address_to))
    api.sync(api.tokens.transfer(entity, address_to, amount, 100))

    from_balance_after = api.tokens.balance(Address(entity))
    assert balance_within_range(from_balance_after, from_balance_before - amount), \
        'Expected balance to decrease by {} - found {} (before); {} (after)'.format(
            amount, from_balance_before, from_balance_after)

    to_balance_after = api.tokens.balance(address_to)
    assert balance_within_range(to_balance_after, to_balance_before + amount), \
        'Expected balance to increase by {} - found {} (before); {} (after)'.format(
            amount, to_balance_before, to_balance_after)


def call_transfer_action_and_verify_balances(api, master_contract, source_contract, signers, target_address, amount):
    from_balance_before = api.tokens.balance(source_contract.address)
    to_balance_before = api.tokens.balance(target_address)

    assert from_balance_before > amount, \
        'Insufficient funds'

    print('Transfer from {} to {} mediated by {}'.format(
        source_contract.address, target_address, master_contract.address))
    api.sync(master_contract.action(
        api, 'execute_transfer', 100, signers, str(source_contract.address), target_address, amount))

    from_balance_after = api.tokens.balance(source_contract.address)
    assert balance_within_range(from_balance_after, from_balance_before - amount), \
        'Expected balance to decrease by {} - found {} (before); {} (after)'.format(
            amount, from_balance_before, from_balance_after)

    to_balance_after = api.tokens.balance(target_address)
    assert balance_within_range(to_balance_after, to_balance_before + amount), \
        'Expected balance to increase by {} - found {} (before); {} (after)'.format(
            amount, to_balance_before, to_balance_after)


def run(options, benefactor):
    api = LedgerApi(options['host'], options['port'])
    entity1, contract1, contract2, master_contract = setup(api)

    transfer_and_verify_balances(api, entity1, contract1.address, 2345)
    call_transfer_action_and_verify_balances(
        api,
        master_contract,
        contract1,
        [entity1],
        contract2.address,
        1345)
    call_transfer_action_and_verify_balances(
        api,
        master_contract,
        contract2,
        [entity1],
        Address(entity1),
        1000)
