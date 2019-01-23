#!/usr/bin/env python3
import argparse
import os
import time
import subprocess
import webbrowser

WATCHDOG_PRESENT = True
try:
  from watchdog.observers import Observer
  from watchdog.events import FileSystemEventHandler
except:
  WATCHDOG_PRESENT = False

SOURCEDIR = 'source'
BUILDDIR = 'build'
DOCUMENTATION_ROOT = os.path.abspath(os.path.dirname(__file__))


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--watch', action='store_true', help='Watch the filesystem')
    parser.add_argument('-b', '--open-browser', action='store_true', help='Open the documentation in the browser')
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


if WATCHDOG_PRESENT:
    class FileSystemWatcher(FileSystemEventHandler):
        def on_any_event(self, event):
            print(event)
            build_documentation()


def watch():
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

    if args.open_browser:
        index_path = os.path.join(DOCUMENTATION_ROOT, BUILDDIR, 'html', 'index.html')
        index_url = 'file://' + index_path.replace(os.sep, '/')
        print('Opening:', index_url)
        webbrowser.open(index_url)

    # watch for update if required
    if (WATCHDOG_PRESENT):
        if (args.watch):
            watch()
    else:
        print("\nWARNING: could not find watchdog package")


if __name__ == '__main__':
    main()