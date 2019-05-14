import sys
import codecs
str = raw_input()
hexlify = codecs.getencoder('hex')
print hexlify(str)
