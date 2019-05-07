import cldoc.clang.cindex
from cldoc.clang.cindex import *

import os
import sys
import re
import glob
import platform
from ctypes.util import find_library

import asciitree
import sys
from jinja2 import Template
import copy
DIR = os.path.abspath(os.path.dirname(__file__))

libclangs = [
    '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/libclang.dylib',
    '/Library/Developer/CommandLineTools/usr/lib/libclang.dylib']

found = False


def printAST(xx):
    print asciitree.draw_tree(xx,
                              lambda n: [x for x in n.get_children()],
                              lambda n: "%s (%s - %s)" % (n.spelling or n.displayname, str(n.kind).split(".")[1], n.type.spelling))


for libclang in libclangs:
    if os.path.exists(libclang):
        cldoc.clang.cindex.Config.set_library_path(os.path.dirname(libclang))
        found = True
        break

if not found:
    print "Could not find clang"
    exit(-1)


def GetConstructors(n):
    ret = []
    for x in n.get_children():
        kind = str(x.kind).split(".")[1]
        if kind == "CONSTRUCTOR":

            # print
            #            print x.access_specifier
            #            print x.availability
            #            print x.kind
            #            print x.type.spelling
            #            print x.semantic_parent.spelling
            #            print x.underlying_typedef_type.spelling
            #            print
            ret.append(
                {"arguments": [q.type.spelling for q in x.get_arguments()]})
    return ret


def GetMethods(n, classname, known_types=None):
    if known_types is None:
        known_types = {}
    ret = {}
    operators = ["=", "==", "!=", "<", ">", "<=", ">=", "+", "-",
                 "/", "*", "+=", "-=", "/=", "*=", "&", "|", "&=", "|="]
    for x in n.get_children():
        kind = str(x.kind).split(".")[1]

        if kind == "CXX_METHOD":
            if x.access_specifier != cldoc.clang.cindex.AccessSpecifier.PUBLIC:
                continue
            if x.is_pure_virtual_method():
                continue
            if x.is_static_method():
                continue

            opname = x.spelling.replace("operator", "")
            is_operator = opname in operators
            opargs = ["py::self", "py::self"]
            def updateType(
                x): return x if not x in known_types else known_types[x]
            arguments = [updateType(q.type.spelling)
                         for q in x.get_arguments()]
            rettype = updateType(x.result_type.spelling)

            if is_operator:
                if len(arguments) != 1:
                    raise BaseException("Expected exactly one argument")
                a = arguments[0]
                if a.endswith("&&"):
                    continue
                if a.startswith("const "):
                    a = a.split("const ", 1)[1]
                if a.endswith("&"):
                    a = a.rsplit("&", 1)[0]
                a = a.replace(" ", "").strip()
                c = classname.replace(" ", "")

                if a != c:
                    opargs[1] = a + "()"

            fullsign = rettype + " (" + classname + "::*)(" + \
                ", ".join(arguments) + ")"
            if x.is_const_method():
                fullsign += " const"

            comment = ""
            detail = {
                "brief": comment,
                "op_args": opargs,
                "is_const": x.is_const_method(),
                "signature": x.type.spelling,
                "full_signature": fullsign,
                "return_type": rettype,
                "arguments": arguments
            }

            if not x.spelling in ret:
                ret[x.spelling] = {
                    "op_name": opname,
                    "is_operator": is_operator,
                    "name": x.spelling,
                    "details": [detail]}
            else:
                ret[x.spelling]["details"].append(detail)

    return ret


def GetBases(n):
    ret = []
    for x in n.get_children():
        kind = str(x.kind).split(".")[1]
        if kind == "CXX_BASE_SPECIFIER":
            ret.append(x.type.spelling)

    return ret


def GetTemplateClassParameters(n):
    ret = []
    for x in n.get_children():
        kind = str(x.kind).split(".")[1]
        if kind == "TEMPLATE_TYPE_PARAMETER":
            ret.append(
                {"name": x.spelling, "definition": "typename " + x.spelling})
        if kind == "TEMPLATE_NON_TYPE_PARAMETER":
            ret.append({"name": x.spelling,
                        "definition": x.type.spelling + " " + x.spelling})
    return ret


def UpdateKnownTypes(n, known_types, short, long):
    for x in n.get_children():
        kind = str(x.kind).split(".")[1]
        if kind == "TYPEDEF_DECL":

            a = x.spelling
            b = x.type.spelling
            val = x.underlying_typedef_type.spelling
            known_types[a] = val
            known_types[b] = val

    return known_types


def DefineClass(n, filename, namespace, is_template=False):

    if not n.extent.start.file.name.endswith(filename):
        return
    tempparam = GetTemplateClassParameters(n)
    qname = n.spelling or n.displayname

    known_types = {}
    short_qclass = "::".join(namespace + [n.spelling or n.displayname])

    if is_template:
        qname = "%s< %s >" % (qname, ", ".join([t["name"] for t in tempparam]))

    long_qclass = "::".join(namespace + [qname])
    known_types[short_qclass] = long_qclass
    known_types = UpdateKnownTypes(n, known_types, short_qclass, long_qclass)

    constructors = GetConstructors(n)
    methods = GetMethods(n, qname, known_types)
    bases = GetBases(n)

    return {
        "is_template": is_template,
        "template_parameters": tempparam,
        "short_template_param_definition": ", ".join([t["definition"] for t in tempparam]),
        "qualified_class_name": qname,
        "bases": bases,
        "namespace": namespace,
        "classname": n.spelling or n.displayname,
        "custom_name": n.spelling or n.displayname,
        "constructors": constructors,
        "methods": methods
    }
#    print "::".join(namespace)
#    print n.spelling or n.displayname
#    print "---"


def Traverse(n, filename, namespace=None, export=None):
    if export is None:
        export = {}
    if namespace is None:
        namespace = []
    namespace = copy.copy(namespace)
    kind = str(n.kind).split(".")[1]

    if kind == "NAMESPACE":
        ns = n.spelling or n.displayname
        if ns == "std":
            return
        namespace.append(ns)

    namestr = "::".join(namespace)
    if not namestr in export:
        export[namestr] = {"namespace": copy.copy(namespace), "classes": []}

    if kind == "CLASS_DECL":
        c = DefineClass(n, filename, namespace)
        if not c is None:
            export[namestr]["classes"].append(c)
    elif kind == "CLASS_TEMPLATE":
        c = DefineClass(n, filename, namespace, True)
        if not c is None:
            export[namestr]["classes"].append(c)
    else:
        for x in n.get_children():
            export = Traverse(x, filename, namespace, export)

    if kind == "NAMESPACE":
        if len(export[namestr]["classes"]) == 0:
            del export[namestr]
    return export


def mkdir_recursive(path):
    sub_path = os.path.dirname(path)
    if not os.path.exists(sub_path):
        mkdir_recursive(sub_path)
    if not os.path.exists(path):
        os.mkdir(path)


def CreatePythonFile(filename, outputdir, translation_unit):
    data = Traverse(translation_unit.cursor, filename)
    shortinc = filename.rsplit(
        "include/", 1)[1] if "include/" in filename else filename
    guard = shortinc
    guard = guard.replace("/", "_").replace(".", "_").upper()

    with open(os.path.join(DIR, "py_bindings.tpp"), "r") as fb:
        t = Template(fb.read())

    includes = [shortinc]
    d, f = shortinc.rsplit("/", 1) if "/" in shortinc else ("", shortinc)
    output = os.path.join(outputdir, d, "py_" + f)

    print "Writing ", filename, " > ", output
    mkdir_recursive(os.path.dirname(output))
    with open(output, "w") as fb:
        fb.write(t.render(includes=includes,
                          HEADER_GUARD=guard, exporting=data, len=len))

    build_functions = []
    for ns, d in data.iteritems():
        for cls in d["classes"]:
            classname = cls["classname"]
            build_functions.append(
                {"namespace": ns, "function": "Build" + classname})

    return {"output": output, "build_functions": build_functions}


if __name__ == "__main__":
    index = cldoc.clang.cindex.Index(
        cldoc.clang.cindex.conf.lib.clang_createIndex(False, True))
    outputdir = sys.argv[2]
    maininc = []
    functions = []
    namespaces = []

    for root, dir, files in os.walk(sys.argv[1]):
        print root
        for f in files:
            if f.endswith(".hpp"):
                filename = os.path.join(root, f)
                addincl = ["-I" + a for a in sys.argv[3:]]
                translation_unit = index.parse(
                    filename, ['-x', 'c++', '-std=c++11', "-I" + sys.argv[1]] + addincl)

                try:
                    a = CreatePythonFile(filename, outputdir, translation_unit)
                    maininc.append(a["output"])
                    bf = a["build_functions"]
                    functions += bf
                except:
                    print "Failed at ", filename
                    print "=" * 80
                    print

    for b in functions:
        namespaces.append(b["namespace"])
    print
    print "//Main file"
    orderedns = []
    createdns = {}

    def create_ns(ns):
        if len(ns) == 0:
            return "module"
        cpp_name = "::".join(ns)
        if cpp_name in createdns:
            return createdns[cpp_name]

        nns = "_".join(ns)
        prev = create_ns(ns[:-1])

        code = "py::module ns_%s = %s.def_submodule(\"%s\")" % (
            nns, prev, ns[-1])
        orderedns.append(code)

        createdns[cpp_name] = "ns_%s" % nns
        return createdns[cpp_name]

    def get_or_create_ns(ns):
        if ns in createdns:
            return createdns[ns]
        parts = ns.split("::")
        return create_ns(parts)

    for f in maininc:
        _, f = f.rsplit("include/", 1)
        print "#include \"%s\"" % f
    for ns in set(namespaces):
        n = get_or_create_ns(ns)
    print "#include <pybind11/pybind11.h>"
    print
    print "PYBIND11_MODULE(libfetchcore, m) {"
    print "  namespace py = pybind11;"
    print
    print "// Namespaces"
    for ns in orderedns:
        print "  ", ns
    print
    print "// Objects"
    for f in functions:
        ff = f["function"]
        ns = f["namespace"]
        print "  ", ff + "(" + createdns[ns] + ")"
    print "}"
    # printAST( translation_unit.cursor )
