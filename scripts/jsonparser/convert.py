hexChars = ['0', '1', '2', '3', '4', '5', '6',
            '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F']


def toHex(c):
    return hexChars[(ord(c) >> 4) & 15] + hexChars[ord(c) & 15]


def encodeHex(str):
    s = "0x"
    for c in reversed(str):
        s += toHex(c)
    return s


def makeCase(s):
    print "case", encodeHex(s) + ":  //", s
    translate = {
        ']': "CloseArray();",
        '}': "CloseObject();",
        ',': "CreateEntry();",
        ':': "CreateProperty();",
        '[': "CreateArray();",
        '{': "CreateObject();"
    }
    for c in s:
        if c in translate:
            print translate[c]
    print "pos+=4;"
    print "continue;"
# makeCase("true")
# makeCase("fals")
# makeCase("null")
# for c1 in [' ', '\n', '\r', '\t']:
#    for c2 in [' ', '\n', '\r', '\t']:
#        makeCase( c1+c2 )

# makeCase("[]")
# makeCase("{}")


# xsexit(0)
makeCase("[\n\r ")
makeCase("[\r\n ")
makeCase("{\n\r ")
makeCase("{\r\n ")

makeCase("],\n\r")
makeCase("],\r\n")
makeCase("},\n\r")
makeCase("},\r\n")

exit(0)
makeCase(": {\n")
makeCase(": {\r")
makeCase(":{\n\r")
makeCase(":{\r\n")
makeCase("}, {")

makeCase(": [\n")
makeCase(": [\r")
makeCase(":[\n\r")
makeCase(":[\r\n")
makeCase("], [")

makeCase("[ ],")
makeCase("[],\n")
makeCase("{},\n")
makeCase("[],\r")
makeCase("{},\r")
