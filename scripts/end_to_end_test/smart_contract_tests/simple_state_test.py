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
persistent x : UInt64;

@action
function set_x_to_3()
  use x;

  assert(!x.existed());
  assert(x.get(123u64) == 123u64);
  assert(!x.existed());

  x.set(1u64);
  assert(x.get(123u64) == 1u64);
  assert(x.get() == 1u64);
  assert(x.existed());

  x.set(2u64);
  assert(x.get(123u64) == 2u64);
  assert(x.get() == 2u64);
  assert(x.existed());

  x.set(3u64);
  assert(x.get(123u64) == 3u64);
  assert(x.get() == 3u64);
  assert(x.existed());
endfunction

@action
function set_x_to_5()
  use x;

  assert(x.existed());
  assert(x.get(123u64) == 3u64);
  assert(x.existed());

  x.set(3u64);
  assert(x.get(123u64) == 3u64);
  assert(x.get() == 3u64);
  assert(x.existed());

  x.set(4u64);
  assert(x.get(123u64) == 4u64);
  assert(x.get() == 4u64);
  assert(x.existed());

  x.set(5u64);
  assert(x.get(123u64) == 5u64);
  assert(x.get() == 5u64);
  assert(x.existed());
endfunction

@query
function check_x() : UInt64
  use x;

  return x.get();
endfunction

@query
function check_x_with_default() : UInt64
  use x;

  return x.get(999u64);
endfunction
"""


def run(options):
    entity1 = Entity()
    api = LedgerApi(options['host'], options['port'])
    api.sync(api.tokens.wealth(entity1, 100000000))
    contract = Contract(CONTRACT_TEXT, entity1)

    api.sync(api.contracts.create(entity1, contract, 10000))

    result = contract.query(api, 'check_x_with_default')
    assert result == 999,\
        'Expected to receive default value of 999, got {}'.format(result)
    try:
        contract.query(api, 'check_x')
        assert False, 'Expected query to fail'
    except RuntimeError:
        pass

    api.sync(contract.action(api, 'set_x_to_3', 200, [entity1]))

    result = contract.query(api, 'check_x_with_default')
    assert result == 3, \
        'Expected to receive value of 3, got {}'.format(result)
    result = contract.query(api, 'check_x')
    assert result == 3, \
        'Expected to receive value of 3, got {}'.format(result)

    api.sync(contract.action(api, 'set_x_to_5', 200, [entity1]))

    result = contract.query(api, 'check_x_with_default')
    assert result == 5, \
        'Expected to receive value of 5, got {}'.format(result)
    result = contract.query(api, 'check_x')
    assert result == 5, \
        'Expected to receive value of 5, got {}'.format(result)
