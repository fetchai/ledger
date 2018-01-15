import sys
import hashlib
str = raw_input()
m = hashlib.sha256()
m.update(str)
print m.hexdigest()
