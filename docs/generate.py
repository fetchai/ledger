#!/usr/bin/env python3
import argparse
import os
import time
import subprocess


SOURCEDIR = 'source'
BUILDDIR = 'build'
DOCUMENTATION_ROOT = os.path.abspath(os.path.dirname(__file__))


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--watch', action='store_true', help='Watch the filesystem')
    return parser.parse_args()


def build_documentation():
    print('Building documentation...')

    cmd = [
        'sphinx-build',
        '-M', 'html',
        SOURCEDIR,
        BUILDDIR
    ]
    subprocess.check_call(cmd, cwd=DOCUMENTATION_ROOT)

    print('Building documentation...complete')



def watch():
    from watchdog.observers import Observer
    from watchdog.events import FileSystemEventHandler

    class FileSystemWatcher(FileSystemEventHandler):
        def on_any_event(self, event):
            print(event)
            build_documentation()

    observer = Observer()
    event_handler = FileSystemWatcher()
    observer.schedule(event_handler, os.path.join(DOCUMENTATION_ROOT, SOURCEDIR), recursive=True)
    observer.start()
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()

    observer.join()


def main():
    

    args = parse_commandline()

    # build the documentation
    build_documentation()

    # watch for update if required
    if args.watch:
        watch()




if __name__ == '__main__':
    main()