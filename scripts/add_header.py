import os
import time
# traverse root directory, and list directories as dirs and files as files
for root, dirs, files in os.walk("./include/"):
    path = root.split(os.sep)
    print((len(path) - 1) * '---', os.path.basename(root))
    for file in files:
        path_to_file = os.path.join(root, file)
        (mode, ino, dev, nlink, uid, gid, size,
         atime, mtime, ctime) = os.stat(path_to_file)
        print(len(path) * '---', file), time.ctime(mtime)
