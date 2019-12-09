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

import base64
import binascii
from fetchai.ledger.api import LedgerApi, submit_json_transaction
from fetchai.ledger.crypto import Entity


def run(options):
    ENDPOINT = 'fetch/token/transfer'
    HOST = options['host']
    PORT = options['port']

    # create the APIs
    api = LedgerApi(HOST, PORT)

    # Hardcoded private keys
    id1PrivateKey = "d222867ac019aa7e5b5946ee23100b4437d54ec08db8e89f6e459d497b579a03"
    id2PrivateKey = "499f40c1bf13e7716e62431b17ab668fa2688b7c94a011a3aab595477bc68347"

    # Create identities from the private keys
    id1 = Entity.from_hex(id1PrivateKey)
    id2 = Entity.from_hex(id2PrivateKey)

    # Load 100000 tokens to id1
    api.sync(api.tokens.wealth(id1, 100000))

    # signed transaction that transfers 2500 FET from id1 to id2. Signed with (r,s) Fees are 1000
    orig_tx = 'a1640000c5ab20e3ab845cb4a1d2c3e4c3b08f5ff42a6ff2a71d7697ba8f32c415b77c7f8d850b3ef025b189a2d9bb4a515a84c3673db6d3ef25385d2c8d1e34b06e2de1c109c46501c103e8565596cd793442c5040bafbc61a08524372f495d9dee08adbc39824e980506947091395cece16636ddc094b9d409d5b34ef0bbd9c99c5caf21fc373802472cf96a8a280f84e833f99240f1c72a58927153d6fdc19f178f69c5b02db29f33541e2b946c78a54ce693c7c81082f22531b85a1707cf290f73fddd2df88681084b0cc7aff38e977215ae5899'
    # signed Malicious transaction of the previous transaction. Signed with (r,n-s)
    mal_tx = 'a1640000c5ab20e3ab845cb4a1d2c3e4c3b08f5ff42a6ff2a71d7697ba8f32c415b77c7f8d850b3ef025b189a2d9bb4a515a84c3673db6d3ef25385d2c8d1e34b06e2de1c109c46501c103e8565596cd793442c5040bafbc61a08524372f495d9dee08adbc39824e980506947091395cece16636ddc094b9d409d5b34ef0bbd9c99c5caf21fc373802472cf96a8a280f84e833f99240cb13a3a05600cd45c7abe860515801941d6eef439e697ab31015543b515e5b85495f2a3f95a352747eda205cc70e0bca7a4d8b1240dfbd43f3a047a1faad30bd'

    # Creating jsons for the above mentioned transactions
    legit_trans = submit_json_transaction(host=HOST, port=PORT, tx_data=dict(
        ver="1.2", data=base64.b64encode(binascii.unhexlify(orig_tx)).decode()), endpoint=ENDPOINT)
    mal_trans = submit_json_transaction(host=HOST, port=PORT, tx_data=dict(
        ver="1.2", data=base64.b64encode(binascii.unhexlify(mal_tx)).decode()), endpoint=ENDPOINT)

    # Sending the transactions to the ledger
    assert legit_trans == mal_trans, "Malleable transactions have different transaction hash"
    api.sync([legit_trans, mal_trans])

    # If transaction malleability is feasible, id2 should have 500 FET.
    # If balance of id2 is more than 250 raise an exception
    assert api.tokens.balance(
        id2) == 2500, "Vulnerable to transaction malleability attack"
