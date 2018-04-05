import cldoc.clang.cindex
from  cldoc.clang.cindex import *

import os, sys, re, glob, platform
from ctypes.util import find_library

import asciitree, sys

libclangs = [
    '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/libclang.dylib',
    '/Library/Developer/CommandLineTools/usr/lib/libclang.dylib'
]

found = False

for libclang in libclangs:
    if os.path.exists(libclang):
        cldoc.clang.cindex.Config.set_library_path(os.path.dirname(libclang))
        found = True
        break

if not found:
    print "Could not find clang"
    exit(-1)

index = cldoc.clang.cindex.Index(cldoc.clang.cindex.conf.lib.clang_createIndex(False, True))
translation_unit = index.parse(sys.argv[1], ['-x', 'c++'])

def DefineClass(n, namespace):
    print "::".join(namespace)
    print n.spelling or n.displayname
    print "---"
    
def Traverse(n, namespace = None):
    if namespace == None: namespace = []
    kind = str(n.kind).split(".")[1]
    print " >>>> ", kind
    if kind == "NAMESPACE":
        namespace.append( n.spelling or n.displayname )

    if kind == "CLASS_DECL":
        DefineClass( n, namespace)
    else:
        print "%s (%s)" % (n.spelling or n.displayname, str(n.kind).split(".")[1])
        for x in n.get_children():
            Traverse(x, namespace)

    if n.kind == "NAMESPACE":
        namespace = namespace[:-1]
        
Traverse( translation_unit.cursor )

#print asciitree.draw_tree(translation_unit.cursor,
#  lambda n: [x for x in n.get_children()],
#  lambda n: "%s (%s)" % (n.spelling or n.displayname, str(n.kind).split(".")[1]))
