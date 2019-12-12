import random

from fetchai.ledger.api import LedgerApi
from fetchai.ledger.contract import Contract
from fetchai.ledger.crypto import Entity


_CONTRACT_TEXT = """\
persistent solution : Int32;

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


class SynergeticContractTestHelper:
    def __init__(self, name, api, entity, workdir="."):
        self._api = api
        self._entity = entity
        self._name = name
        self.contract = None
        self._workdir = workdir

    def create_new(self, fee_limit):
        self.contract = Contract(_CONTRACT_TEXT, self._entity)
        print('Creating contract..')
        self._api.sync(self._api.contracts.create(
            self._entity, self.contract, fee_limit))
        if len(self._name) > 0:
            with open(self._workdir+"/"+self._name+".json", "w") as f:
                self.contract.dump(f)

    def load(self):
        print('Loading contract..')
        with open(self._workdir+"/"+self._name+".json", "r") as f:
            self.contract = Contract.load(f)

    def submit_random_data(self, n, number_range, hold_state_sec=10):
        # create a whole series of random data to submit to the DAG
        random_ints = [random.randint(*number_range) for _ in range(n)]
        txs = [self._api.contracts.submit_data(self._entity, self.contract.address, value=value)
               for value in random_ints]
        self._api.sync(txs, hold_state_sec=hold_state_sec,
                       extend_success_status=["Submitted"])
        print('Data submitted.')

    def validate_execution(self):
        result = self.contract.query(self._api, 'query_result')
        return result != -1
