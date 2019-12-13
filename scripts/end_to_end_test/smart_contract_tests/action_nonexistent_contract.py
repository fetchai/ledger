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
@action
function some_action()
endfunction
"""


def run(options, benefactor):
    entity1 = Entity()
    api = LedgerApi(options['host'], options['port'])

    api.sync(api.tokens.transfer(benefactor, entity1, 100000, 1000))

    contract = Contract(CONTRACT_TEXT, entity1)

    try:
        api.sync([contract.action(api, 'some_action', 100, [entity1])])
        assert False, 'Expected action to fail'
    except RuntimeError as e:
        assert 'Contract Lookup Failure' in str(e), \
            'Unexpected error message from server: {}'.format(e)
