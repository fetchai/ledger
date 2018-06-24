#!/usr/bin/env python3
from __future__ import print_function

import os
import re
import sys
import syslog
import traceback

from utils.stringUtils import isstr

FLAG_INFO_valuename=0
FLAG_INFO_valuenames=1
FLAG_INFO_default=2
FLAG_INFO_required=3
FLAG_INFO_typename=4
FLAG_INFO_helptext=5
FLAG_INFO_bogus_flag=6
FLAG_INFO_deprecated_flag=7
FLAG_INFO_varname=8
FLAG_INFO_text=9

__flag_info = {}

class FlagRep(object):
    def __init__(self, *v):
        super(FlagRep, self).__init__()
        self.values = v

    def __getitem__(self, key):
        return self.values[key]


class Search(object):
    def __init__(self):
        pass

g_debug_flags = "--debug-flags" in sys.argv
g_doclines = []

AUTOFLAG = Search()

def debugFlagMessage(m):
    if g_debug_flags:
        print('flagUtils: ' + m, file=sys.stderr)

class FlagSoftwareError(Exception):
    def __init__(self, m=''):
        super(FlagSoftwareError, self).__init__(m + ' (This is a code error. Contact a developer.)')
        debugFlagMessage(str(self))

class FlagRuntimeError(Exception):
    def __init__(self, m=''):
        super(FlagRuntimeError, self).__init__(m + " (This is an error caused by the value you gave. See the program's docs)")
        debugFlagMessage(str(self))

class FlagDeclaredInWrongPlace(FlagSoftwareError):
    def __init__(self):
        super(FlagDeclaredInWrongPlace, self).__init__('Flags must only be defined at module level.')

class FlagBadInjectedCache(FlagSoftwareError):
    def __init__(self):
        super(FlagBadInjectedCache, self).__init__('Passing a cache is supported only in tests.')

class FlagBadDuplication(FlagSoftwareError):
    def __init__(self, m=''):
        super(FlagBadDuplication, self).__init__('Flag %s is duplicated with different valuenames' % str(m))

class FlagMissing(FlagRuntimeError):
    def __init__(self, m=''):
        super(FlagMissing, self).__init__('Flag %s is required and not provided.' % _stringify(m))

class FlagUnexpected(FlagRuntimeError):
    def __init__(self, m=''):
        super(FlagUnexpected, self).__init__('Flag %s is provided but unexpected.' % _stringify(m))

class FlagBogusOption(FlagRuntimeError):
    def __init__(self, m=''):
        super(FlagBogusOption, self).__init__('Flag %s  is not a valid option.' % m)

class FlagValueNotConvertToType(FlagRuntimeError):
    def __init__(self, valuename, value, typename, varname):
        super(FlagValueNotConvertToType, self).__init__(valuename + ' of "' + str(value) + '" cannot be converted to type ' + str(typename) +' for ' + varname)

class FlagClashesWithGlobalName(FlagSoftwareError):
    def __init__(self, varname):
        super(FlagClashesWithGlobalName, self).__init__('Flags must not cause global name collisions:' + varname)

class FlagBadlyNamed(FlagSoftwareError):
    def __init__(self, m=''):
        super(FlagBadlyNamed, self).__init__(m)

def _listify(x):
    if isinstance(x, list):
        return x
    return [x]

import builtins
BUILTINS = builtins

def setFlagValue(varname, value):
    setattr(
        BUILTINS,
        varname,
        value
    )

def flagExists(varname):
    return hasattr(BUILTINS, varname)

def flagList():
    print("FLAGS:" + ",".join(
        x for x in dir(BUILTINS) if x[:2] == "g_"
    ))

def getFlagValue(varname):
    if flagExists(varname):
        return getattr(BUILTINS, varname)
    return None

def printableFlagSet(flagreps):
    return ",".join([x[FLAG_INFO_varname] for x in flagreps])

def _stringify(x, j=','):
    if isinstance(x, list):
        return j.join(x)
    return str(x)

def formatFlagHelp(f):
    a = '  '
    a += (','.join(_listify(f[FLAG_INFO_valuenames]))+' ' *40)[0:40]
    a += ' --'
    a += ' (def='+str(f[FLAG_INFO_default])+')' if f[FLAG_INFO_default] else ''
    a += ' REQUIRED.' if f[FLAG_INFO_required] else ''
    a += ' Type '+f[FLAG_INFO_typename].__name__+'.' if f[FLAG_INFO_typename] else ''
    a += ' ' + str(f[FLAG_INFO_helptext])

    if f[FLAG_INFO_bogus_flag]:
        a += ' [INVALID!!]'
    if f[FLAG_INFO_deprecated_flag]:
        a += ' [DEPRECATED!]'
    return a

# Builds the whole help message from the map of __flag_info.
def helpMessage():
    results = []

    if g_doclines:
        results.extend(g_doclines)

    results.extend([
        'Flags with uppercase names can be searched for in environment or config.',
        'Flags can be given a value on commandline by saying flagname=value or --flagname=value.',
        'Flags can be given a true value with --flag or false value with --no_flag or --no-flag.',
        '',
    ])

    for f in sorted(__flag_info.keys()):
        results.append(formatFlagHelp(__flag_info[f]))
    results.extend([
        '',
    ])
    return results

def formatFlagSummary(f):
    a = ''
    a += (str(f[FLAG_INFO_varname])+' ' *40)[0:40]
    a += ' => '
    a += '"' + str(f[FLAG_INFO_text]) + '"'
    a += ' (from ' + str(f[FLAG_INFO_locn]) + ')'
    return a


# Builds the whole summary message from the map of __flag_info.
def summaryMessage():
    results = []

    for f in sorted(__flag_info.keys()):
        results.append(formatFlagSummary(__flag_info[f]))

    return results


# Hurray!!! Our MAIN ENTRY POINT!
# Call it like this;
#
#   Flag(g_myflag=['MYFLAG', 'myflag', 'my_flag'], default=42, required=True,
#        deprecated=False, bogus=False, help="My flag for passing in my setting!")
#
# Most of that should be self explanatory, except:
#   Bogus flags have been removed. We still parse them (so they're eaten from ARGV), but complain
#   about them being used. Apps can enable a throw if they're used.
#
#   Deprecated flags are ones which should be being used. They warn on use.
#
#   The "help" setting is the message which is produced to try and tell the user how to use the
#   flag. note that you don't need to include default values, requiredness etc since they're
#   added for you by the help system.

def Flag(**parms):
    if (traceback.extract_stack()[-2][2] != '<module>'
            and not _flag_utils_inside_test):
        syslog.syslog(syslog.LOG_CRIT, 'Flags must only be defined at module level')
        raise FlagDeclaredInWrongPlace()

    if g_debug_flags:
        debugFlagMessage("FLAG:"+str(parms))

    varname = None
    valuename = None
    default = None
    required = False
    typename = None
    src_string = False
    helptext = '<no help available>'
    bogus_flag = False
    deprecated_flag = False

    # We iterate across the KW args passed to us. If the X of the X=Y
    # is something specific, we grab the Y.
    # if it's NOT then it's the variable name (the "g_myflag") so we
    # save it and the X will then be the sourcenames so we save those.

    for (x, y) in parms.items():
        if x == 'default':
            default = y
        elif x == 'required':
            required = y
        elif x == 'type':
            typename = y
        elif x == 'deprecated':
            deprecated_flag = y
        elif x == 'bogus':
            bogus_flag = y
        elif x == 'help':
            helptext = y
        else:
            varname = x
            valuename = y

    if not varname:
        raise FlagBadlyNamed('Flags must have a name.')

    if flagExists(varname) and not re.match(r'g_[a-z_]*', varname):
        raise FlagClashesWithGlobalName(varname)

    if varname in __flag_info:

        # If the flag has the same name as one we've already tried to create, then
        # we should check if it looks like an alias or a clash. If the valuenames string
        # is the same, we'll consider it an alias and silently leave, the processing
        # has been done in the earlier call.
        # If not, we throw a complaint about the name clash.

        if (
            sorted(_listify(valuename)) != sorted(_listify(__flag_info[varname][FLAG_INFO_valuenames]))
            or
            default != __flag_info[varname][FLAG_INFO_default]
            or
            typename != __flag_info[varname][FLAG_INFO_typename]
            or
            bogus_flag != __flag_info[varname][FLAG_INFO_bogus_flag]
        ):
            raise FlagBadDuplication(varname)
        return

    if isinstance(valuename, Search):
        if varname[0:2] == 'g_':
            varname = varname[2:]
        argname = varname.lower().replace('_', '-')
        envname = varname.upper().replace('-', '_')
        varname = varname.lower().replace('-', '_')
        if varname[0:2] != 'g_':
            varname = 'g_' + varname
        valuename = [ argname, envname ]

    found = False
    valuenames = _listify(valuename)

    info = FlagRep(
        valuename,
        valuenames,
        default,
        required,
        typename,
        helptext,
        bogus_flag,
        deprecated_flag,
        varname
    )
    __flag_info[varname] = info
    debugFlagMessage("ALL NOW:" + printableFlagSet(__flag_info.values()))
    return

def handle(flag, value):
    done = False
    debugFlagMessage("HANDLE %s?" % (flag[FLAG_INFO_valuenames]))
    var = flag[FLAG_INFO_varname]

    if isstr(value) and flag[FLAG_INFO_typename]:
        try:
            if flag[FLAG_INFO_typename] == list:
                value = value.split(',')
            else:
                value = flag[FLAG_INFO_typename](value)
        except (TypeError, ValueError) as x:
            raise FlagValueNotConvertToType(_stringify(valuenames), value, flag[FLAG_INFO_typename], flag[FLAG_INFO_varname])

    debugFlagMessage("HANDLE %s => %s" % (var,value))
    setFlagValue(var, value);
    return flag

def handleKV(k, v):
    for f in __flag_info.values():
        for n in f[FLAG_INFO_valuenames]:
            if n == k:
                return handle(f, v)
    raise FlagUnexpected(k)

def searchEnviron(missingFlags):
    result = []
    for missFlag in missingFlags:
        var = missFlag[FLAG_INFO_varname]
        for cand in missFlag[FLAG_INFO_valuenames]:
            if cand in os.environ:
                v = os.environ[cand]
                result.append(handle(missFlag, v))
                break
    return result


Flag(g_show_help="help", help="Show help")
Flag(debug_flags="debug-flags", help="Turn on flag debugging.")

def startFlags(args):
    assigned = set()
    words = []

    for flag in __flag_info.values():
        if not flagExists(flag[FLAG_INFO_varname]):
            setFlagValue(flag[FLAG_INFO_varname], None)

    defaultables = [
        flag
        for flag in  __flag_info.values()
        if flag[FLAG_INFO_default] != None
    ]
    debugFlagMessage("ALL:" + printableFlagSet(__flag_info.values()))
    debugFlagMessage("DEFAULTING:" + printableFlagSet(defaultables))

    assigned.update([
        handle(flag, flag[FLAG_INFO_default])
        for flag in defaultables
    ])

    for arg in args[1:]:
        if arg:
            done = False
            # Try matching these regexes against the ARGV.
            # If it matches, the second element gives you either
            # - a group to pull out of the regex for the value (if an integer).
            # - a value to use (if not an integer).
            for lookingfor in [(r'--([-\w]+)=(.*)', 2),
                               (r'([-\w]+)=(.*)', 2),
                               (r'--no[-_]([-\w]+)', False),
                               (r'--([-\w]+)', True)]:
                prog = re.compile(lookingfor[0])
                m = prog.match(str(arg))
                # Check we have a match and the 1st capture is the name we're seeking.
                if m:
                    if lookingfor[1] in [ True, False ]:
                        assigned.add(handleKV(m.group(1), lookingfor[1]))
                        # We found a boolean setting for the flag.
                        done = True
                        break

                    elif isinstance(lookingfor[1], int):
                        # We found an expression which uses the Y of --X=Y
                        # as the value of the flag.
                        assigned.add(handleKV(m.group(1), m.group(lookingfor[1])))
                        done = True
                        break

                    else:
                        # If it's anything else, then we use the second value of
                        # the tuple as the flag value.
                        assigned.add(handleKV(m.group(1), lookingfor[1]))
                        done = True
                        break

        if not done:
            debugFlagMessage("PLAIN %s" % (arg))
            words.append(arg)

    if getFlagValue("g_show_help"):
        print('\n'.join(helpMessage()))
        exit(0)

    required = [ x for x in __flag_info.values() if x[FLAG_INFO_required] ]
    missing = set(required) - set(assigned)
    assigned.update(searchEnviron(missing))
    missing = [x[FLAG_INFO_valuenames][0] for x in (set(required) - set(assigned))]

    debugFlagMessage("ASSIGNED:" + printableFlagSet(assigned))
    debugFlagMessage("MISSING :" + printableFlagSet(missing))
    debugFlagMessage("REQUIRED:" + printableFlagSet(required))

    if missing:
        raise FlagMissing(missing)
    return words
