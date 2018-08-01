import requests
import itertools
import threading
from multiprocessing import Pool
import time
import json

from utils.messages import title, note, text, info, debug, progress, warn, error, fatal

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
                if r.content:
                    data = json.loads(r.content.decode("utf-8", "strict"))
                else:
                    data = None
            else:
                data = None
        except requests.exceptions.Timeout as ex:
            data = None
            code = -1
            warn("Timeout:", ident)
        except requests.exceptions.ConnectionError as ex:
            data = None
            info("Denied:", ident)
            code = -2

    except Exception as x:
        print("ERR:", x)
        code = -4
        exit(77)
    time.sleep(0.1)
    return (nodenumber, ident, url, code, data)


def doTask(task):
    return poll(task[1], task[0]) 

class Getter(object):

    def __init__(
            self,
            owner,
            nodeRangeGenerator,
            actions
            ):
        self.nodeRangeGenerator = nodeRangeGenerator
        self.actions = actions
        self.thread = Getter.WorkerThread(self)
        self.owner = owner

    def start(self):
        self.thread.start()

    def stop(self):
        self.thread.stop()

    class WorkerThread(threading.Thread):
        def __init__(self, owner):
            self.done = False
            self.owner = owner
            self.port = 0
            super().__init__(group=None, target=None, name="pollingthread")
            self.myPool = Pool(20)

        def stop(self):
            self.done = True

        def run(self):
            print("MONITORING START")

            while not self.done:
                time.sleep(.25)
                idents = list(self.owner.nodeRangeGenerator.getall())
                urls = [ x for x in self.owner.actions.keys() if isinstance(x, str) ]

                tasks = itertools.product(idents, urls)
                newdata = self.myPool.map(doTask, tasks)

                for newstuff in newdata:
                    if newstuff[3] == 200:
                        func = self.owner.actions.get(200, None)
                        if func:
                            func(*newstuff)
                        func = self.owner.actions.get(newstuff[2], None)
                        if func:
                            func(*newstuff)
                    else:
                        func = self.owner.actions.get(None, None)
                        if func:
                            func(*newstuff)
                self.owner.owner.complete()
