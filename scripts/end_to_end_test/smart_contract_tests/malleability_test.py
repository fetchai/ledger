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

from fetchai.ledger.api import LedgerApi, submit_json_transaction
from fetchai.ledger.crypto import Identity, Entity


def run(options):
    ENDPOINT = 'transfer'
    # create the APIs
    api = LedgerApi(options['host'], options['port'])

    # Hardcoded private keys
    id1PrivateKey = "d222867ac019aa7e5b5946ee23100b4437d54ec08db8e89f6e459d497b579a03"
    id2PrivateKey = "499f40c1bf13e7716e62431b17ab668fa2688b7c94a011a3aab595477bc68347"

    # Create identities from the private keys
    id1 = Entity.from_hex(id1PrivateKey)
    id2 = Entity.from_hex(id2PrivateKey)

    # Load 1000 tokens to id1
    api.sync(api.tokens.wealth(id1, 1000))

    # transaction payload that transfers 250 FET from id1 to id2. Signed with (r,s)
    orig_tx = b'\xa1D\x00\x00\xc5\xab \xe3\xab\x84\\\xb4\xa1\xd2\xc3\xe4\xc3\xb0\x8f_\xf4*o\xf2\xa7\x1dv\x97\xba\x8f2\xc4\x15\xb7|\x7f\x8d\x85\x0b>\xf0%\xb1\x89\xa2\xd9\xbbJQZ\x84\xc3g=\xb6\xd3\xef%8],\x8d\x1e4\xb0n-\xe1\xc0\xfa\xc0\x85\x01\x14\x00\x00\x00\x00\x00\x00\x00\x00\x04\x0b\xaf\xbca\xa0\x85$7/I]\x9d\xee\x08\xad\xbc9\x82N\x98\x05\x06\x94p\x919\\\xec\xe1f6\xdd\xc0\x94\xb9\xd4\t\xd5\xb3N\xf0\xbb\xd9\xc9\x9c\\\xaf!\xfc78\x02G,\xf9j\x8a(\x0f\x84\xe83\xf9\x92@!\x92\xa0\xa1%\xd5Q\xb6\x08\x00\xd4A\xcb4\x83\xcd6W<"\xa7?"V=m\xd7\xb2~g{\x98\xbaN/wYh\x88\x83\x9a<o$9\xc9yI\xea(\x92?\x16\x8d6\nm\x15\\+\xe7\x95p\xaf'
    # Malicious transaction payload of the previous transaction. Signed with (r,n-s)
    mal_tx = b"\xa1D\x00\x00\xc5\xab \xe3\xab\x84\\\xb4\xa1\xd2\xc3\xe4\xc3\xb0\x8f_\xf4*o\xf2\xa7\x1dv\x97\xba\x8f2\xc4\x15\xb7|\x7f\x8d\x85\x0b>\xf0%\xb1\x89\xa2\xd9\xbbJQZ\x84\xc3g=\xb6\xd3\xef%8],\x8d\x1e4\xb0n-\xe1\xc0\xfa\xc0\x85\x01\x14\x00\x00\x00\x00\x00\x00\x00\x00\x04\x0b\xaf\xbca\xa0\x85$7/I]\x9d\xee\x08\xad\xbc9\x82N\x98\x05\x06\x94p\x919\\\xec\xe1f6\xdd\xc0\x94\xb9\xd4\t\xd5\xb3N\xf0\xbb\xd9\xc9\x9c\\\xaf!\xfc78\x02G,\xf9j\x8a(\x0f\x84\xe83\xf9\x92@\xf1\xa01\x1c\xc9\xb4.\xda\xa4\x1dS~=\x1f\x9dT\xac\x15\x95\x04'l'\xbe)s\xce\xa8\\\x8e\x8a\x17\x14\x8a+\x1d\xc3\xe1\x88\xf9\x11\xc6\xb0_\x16=\x95\xb9d\xaeYJ4xp\x97>Y\xb3\xc9=5\xb8\x95"

    # Creating jsons for the above mentioned transactions
    legit_trans = api.tokens._post_tx_json(orig_tx, ENDPOINT)
    mal_trans = api.tokens._post_tx_json(mal_tx, ENDPOINT)

    # Sending the transactions to the ledger
    assert legit_trans == mal_trans, "Malleable transactions have different transaction hash"
    api.sync([legit_trans, mal_trans])

    # If transaction malleability is feasible, id2 should have 500 FET.
    # If balance of id2 is more than 250 raise an exception
    assert api.tokens.balance(
        id2) == 250, "Vulnerable to transaction malleability attack"
