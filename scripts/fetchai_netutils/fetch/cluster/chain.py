from fetchai.ledger.api.common import ApiEndpoint


class ChainStatus(ApiEndpoint):
    def get_last_n(self, n: int, fields="*"):
        status, response = self._get_json("status/chain")
        if not status:
            return {}
        chain = response["chain"][:n]
        if fields == "*":
            return chain
        return list(map(lambda x, fs=fields: {f: x[f] for f in fs}, chain))


class ChainSyncTesting:
    def __init__(self, nodes):
        self._apis = []
        self._idx_map = {}
        i = 0
        for n in nodes:
            self._apis.append(ChainStatus(n["host"], n["port"]))
            self._idx_map["{}:{}".format(n["host"], n["port"])] = i
            i += 1

    def _get_chain(self, n):
        resps = []
        for api in self._apis:
            resps.append(api.get_last_n(n, ["hash"]))
        resps = [list(map(lambda x: x["hash"], resp)) for resp in resps]
        return resps

    def network_synced(self, depth=5):
        resps = self._get_chain(depth)
        for i in range(len(resps)):
            for j in range(len(resps)):
                if i == j:
                    continue
                try:
                    resps[j].index(resps[i][0])
                except ValueError:
                    return False
        return True

    def node_synced(self, host, port, depth=5):
        idx = self._idx_map["{}:{}".format(host, port)]
        resps = self._get_chain(depth)
        for i in range(len(resps)):
            if i == idx:
                continue
            try:
                resps[idx].index(resps[i][0])
            except ValueError:
                return False
        return True
