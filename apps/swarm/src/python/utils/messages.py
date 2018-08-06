import sys
import traceback
from contextlib import contextmanager

from utils.colours import colours
from utils import flags


flags.Flag(g_verbose = flags.AUTOFLAG, default = False, help = "verbose")
flags.Flag(g_debug = flags.AUTOFLAG, default = False, help = "debug")

prefixes = []

@contextmanager
def MessagesPrefix(prefix):
    global prefixes
    prefixes.append(str(prefix))
    yield prefix
    prefixes.pop()

def getLinePrefix(pref):
    r = ""
    if pref and len(str(pref)) > 0:
        r += str(pref)
        if pref[:-1] != ":":
            pref += ":"
    if len(prefixes) > 0:
        r += "  ".join(prefixes)
        r += "  "
    return r

def _containsNL(target):
    if isinstance(target, str):
        return '\n' in target
    return False

def processMessageList(pref, *messages):
    if any([_containsNL(message) for message in messages]):
        res = []
        for message in messages:
            res.extend( message.split("\n") )
        return [ getLinePrefix(pref) + str(x) + "\n" for x in res ]
    else:
        return [ getLinePrefix(pref) ] + [ str(m) for m in messages ]

def formatLog(exctype, value, tb):
    s = []
    lines = traceback.format_exception(exctype, value, tb)
    lines.pop(0)

    for line in reversed(lines):
        line = str(line.strip('\n \t\r').partition('\n')[0])
        if line.startswith('File '):
            line = 'AT: %s' % line
        s.append(line)
    return s

def title(*messages):
    print(colours.inColour("underline", *processMessageList( "* ", *messages)))
    return 1

def text(*messages):
    print(colours.inColour("", *processMessageList( "  ", *messages)))
    return 1

def debug(*messages):
    if g_debug:
        print(colours.cyan(*processMessageList( "D:", *messages)))
    return 0

def info(*messages):
    if g_verbose or g_debug:
        print(colours.darkblue(*processMessageList( "I:", *messages)))
    return 0

def note(*messages):
    print(colours.darkmagenta(*processMessageList( "I:", *messages)))
    return 0

def progress(*messages):
    print(colours.darkgreen(*processMessageList( "I:", *messages)))
    return 0

def warn(*messages):
    print(colours.darkyellow(*processMessageList( "W:", *messages)))
    return 1

def error(*messages):
    print(colours.darkred(*processMessageList( "E:", *messages)))
    return 1

def unfatal(*messages):
    if any([isinstance(x, Exception) for x in messages]):
        (x,v,t) = sys.exc_info()
        s = formatLog(x,v,t)

        outputs = [ "\n".join(s) ] + [ str(m) for m in messages if not isinstance(m, Exception) ]
        print(colours.red(*processMessageList( "FAIL:", *outputs)))
    print(colours.red(*processMessageList( "FAIL:", *messages)))

def fatal(*messages):
    unfatal(*messages)
    sys.exit(1)
