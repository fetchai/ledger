import os
import threading
import time


class MonitoredLogFile(object):
    MAX_LINES_TO_READ = 10

    def __init__(self, path, prefix):
        self._path = str(path)
        self._prefix = str(prefix)
        self._offset = 0

    def update(self):

        # skip over when the log file has not been made yet
        if not os.path.isfile(self._path):
            return

        with open(self._path, 'r') as log_file:
            log_file.seek(self._offset)

            for _ in range(self.MAX_LINES_TO_READ):

                # attempt to read another line
                last_offset = self._offset
                try:
                    line = log_file.readline()
                except:
                    line = "UNREADABLE_LINE"
                    pass
                self._offset = log_file.tell()

                # in the case when we could not read any more line data
                if last_offset == self._offset:
                    break

                # display the output
                # print('{:6d} {}: {}'.format(self._offset, self._prefix, line.rstrip()))
                print('{}: {}'.format(self._prefix, line.rstrip()))


class ConstellationMonitor(object):
    def __init__(self, nodes):
        self._nodes = nodes
        self._log_files = []
        self._running = False
        self._thread = None

    def start(self):

        # kick off the thread
        self._running = True
        self._thread = threading.Thread(target=self._background_monitor)
        self._thread.daemon = True
        self._thread.start()

    def stop(self):
        self._running = False
        self._thread.join()

    def _background_monitor(self):

        # create the log file watchers
        monitors = [MonitoredLogFile(n.log_path, n.p2p_address)
                    for n in self._nodes]

        while self._running:
            for monitor in monitors:
                monitor.update()

            time.sleep(0.35)
