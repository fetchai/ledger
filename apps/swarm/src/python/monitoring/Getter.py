import requests
import itertools
import threading
from multiprocessing import Pool
import time
import json

def poll(url, nodenumber):
    ident = "127.0.0.1:{}".format(nodenumber + 9000)
    port = nodenumber + 10000
    code = -3
    data = None
    try:
        fullurl = "http://127.0.0.1:{}{}".format(port, url)
        data = None
        try:
            r = requests.get(fullurl, timeout=100)
            code = r.status_code
            if code == 200:
                data = json.loads(r.content.decode("utf-8", "strict"))
            else:
                data = None
        except requests.exceptions.Timeout as ex:
            data = None
            code = -1
            print("Timeout:", ident)
        except requests.exceptions.ConnectionError as ex:
            data = None
            print("Denied:", ident)
            code = -2

    except Exception as x:
        print("ERR:", x)
        code = -4
    time.sleep(0.1)
    return (nodenumber, ident, url, code, data)


def doTask(task):
    return poll(task[1], task[0]) 

class Getter(object):

    def __init__(
            self,
            nodeRangeGenerator,
            actions
            ):
        self.nodeRangeGenerator = nodeRangeGenerator
        self.actions = actions
        self.thread = Getter.WorkerThread(self)

    def start(self):
        self.thread.start()

    class WorkerThread(threading.Thread):
        def __init__(self, owner):
            self.done = False
            self.owner = owner
            self.port = 0
            super().__init__(group=None, target=None, name="pollingthread")
            self.myPool = Pool(20)

        def run(self):
            print("MONITORING START")

            while not self.done:
                time.sleep(.25)
                idents = list(self.owner.nodeRangeGenerator.getall())
                urls = self.owner.actions.keys()

                tasks = itertools.product(idents, urls)
                newdata = self.myPool.map(doTask, tasks)

                for newstuff in newdata:
                    if newstuff[3] == 200:
                        func = self.owner.actions.get(newstuff[2], None)
                        if func:
                            func(*newstuff)
