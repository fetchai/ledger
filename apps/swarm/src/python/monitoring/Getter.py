import requests
import itertools
import threading
from multiprocessing import Pool
import time
import json

from utils.messages import title, note, text, info, debug, progress, warn, error, fatal

def poll(url, path, nodenumber):
    ident = nodenumber
    code = -3
    data = None
    try:
        fullurl = url
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
    print("GET:", url, code)
    return (nodenumber, ident, path, code, data)


def doTask(task):
    url = task.get("url")
    path = task.get("path")
    ident = task.get("ident")
    return poll(url, path, ident)


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

            idents = list(self.owner.nodeRangeGenerator.getall())
            paths = [ x for x in self.owner.actions.keys() if isinstance(x, str) ]
            items = list(itertools.product(idents, paths))
            tasks = []
            for i,p in items:
                t = {}
                t.update(i)
                t["path"] = p
                if "port" not in t:
                    t["port"] = 8000
                if "url" not in t:
                    t["url"] = "127.0.0.1:{}".format(t["port"])
                t["url"] = t["url"] + p
                tasks.append(t)

            print("MONITORING START")

            while not self.done:
                time.sleep(.25)
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
