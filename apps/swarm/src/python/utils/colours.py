import functools

class Colours(object):
        COLOUR_CONTROL_CODES = {
            'white'       : "\033[1;37m",
            'yellow'      : "\033[1;33m",
            'green'       : "\033[1;32m",
            'blue'        : "\033[1;34m",
            'cyan'        : "\033[1;36m",
            'red'         : "\033[1;31m",
            'magenta'     : "\033[1;35m",
            'black'       : "\033[1;30m",
            'darkwhite'   : "\033[37m",
            'darkyellow'  : "\033[33m",
            'darkgreen'   : "\033[32m",
            'darkblue'    : "\033[34m",
            'darkcyan'    : "\033[36m",
            'darkred'     : "\033[31m",
            'darkmagenta' : "\033[35m",
            'darkblack'   : "\033[30m",
            'off'         : "\033[0m",

            'on-white'       : "\033[47m",
            'on-yellow'      : "\033[43m",
            'on-green'       : "\033[42m",
            'on-blue'        : "\033[44m",
            'on-cyan'        : "\033[46m",
            'on-red'         : "\033[41m",
            'on-magenta'     : "\033[45m",
            'on-black'       : "\033[40m",

            'blinking'   : "\033[5m",
            'blink'   : "\033[5m",
            'underline'   : "\033[4m",
            'emph'   : "\033[3m",
                }

        def __init__(self):
            for k,v in self.COLOUR_CONTROL_CODES.items():
                setattr(self, k, functools.partial(self.inColourThunk, k))

        def inColourThunk(self, colname, *args):
            return self.inColour(colname, *args)

        def startInColour(self, c):
            r = ''
            if c:
                if '.' in c:
                    for cc in c.split('.'):
                        r += self.COLOUR_CONTROL_CODES[cc]
                else:
                    r += self.COLOUR_CONTROL_CODES[c]
            return r

        def inColour(self, c, *x):
            r = self.startInColour(c)
            r += ''.join([str(xx) for xx in x])
            if r.endswith('\n'):
                r = r [:-1]
            if c:
                r += self.COLOUR_CONTROL_CODES['off']
            return r

colours = Colours()
