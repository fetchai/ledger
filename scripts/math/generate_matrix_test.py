import numpy as np
types = ["int", "float", "double"]


def randi(*args):
    return np.random.randint(-10, 10, size=args)


rngs = {"int": randi, "float": np.random.randn, "double": np.random.randn}

embodiments = {
    "function": "R.%s(A,B).AllClose(C)",
    "op": "(A %s B).AllClose(C)",
    "inline_op": "(R = A, R %s B).AllClose(C)",
    "inline_function": "( R = A, R.%s(B) ).AllClose(C)"
}

tests = {
    '+': ("Addition", "Add", [], []),
    '*': ("Multiplication", "Multiply", [], []),
    '-': ("Subtraction", "Subtract", [], []),
    '/': ("Division", "Divide", ["int"], []),
    'dp': ("Dot product", "Dot", [], ["op", "inline_op"])
}


for type in types:
    rng = rngs[type]
    for op, details in tests.iteritems():
        test_title, function, exclude, ignore = details

        if type in exclude:
            break

        iop = op + "="
        ifunction = "Inline" + function

        names = {
            "function": function,
            "op": op,
            "inline_op": iop,
            "inline_function": ifunction
        }

        n = 7
        m = 7
        A = rng(n, m)
        B = rng(n, m)
        if op == "+":
            C = A + B
        elif op == "/":
            C = A / B
        elif op == "-":
            C = A - B
        elif op == "*":
            C = A * B
        elif op == "dp":
            C = np.dot(A, B)

        m1 = " ;\n".join([" ".join([str(y) for y in x]) for x in A])
        m2 = " ;\n".join([" ".join([str(y) for y in x]) for x in B])
        m3 = " ;\n".join([" ".join([str(y) for y in x]) for x in C])

        print """
        SCENARIO("%s") {

        _M<%s> A,B,C,R;

        R.Resize( %d, %d );
        A = _M<%s>(R\"(\n%s\n)\");
        B = _M<%s>(R\"(\n%s\n)\");
        C = _M<%s>(R\"(\n%s\n)\");
        """ % (test_title + " for " + type, type, n, m, type, m1, type, m2, type, m3)

        for method, emb in embodiments.iteritems():
            if method in ignore:
                continue
            name = names[method]
            tt = emb % name
            print "EXPECT( %s );" % tt

        print "};"
        print
