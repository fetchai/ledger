import cldoc.clang.cindex
from cldoc.clang.cindex import Config, TypeKind, CursorKind, Index
import re
import asciitree


def node_children(node):
    return (c for c in node.get_children())


def print_node(node):
    text = node.spelling or node.displayname
    kind = str(node.kind)[str(node.kind).index('.')+1:]
    return '{} {}'.format(kind, text)


def print_tree(node):
    print(asciitree.draw_tree(node, node_children, print_node))


class StyleChecker(object):

    def __init__(self,  include_dirs=None, compile_args=None):
        include_dirs = include_dirs or []
        compile_args = compile_args or [
            '-x', 'c++', "-std=c++14", "-Wno-pragma-once-outside-header"]

        self._index = cldoc.clang.cindex.Index(
            cldoc.clang.cindex.conf.lib.clang_createIndex(False, False))

        self._include_dirs = include_dirs
        self._compile_args = compile_args + ["-I" + x for x in include_dirs]

        self._filename = None
        self._filedata = []
        self.has_error = False
        self.total_errors = 0

    def ClearErrorFlag(self):
        self.has_error = False
        self.total_errors = 0

    def __call__(self, filename):
        self._filename = filename.strip()

        with open(filename, "r") as fb:
            self._filedata = fb.readlines()

        translation_unit = self._index.parse(
            filename, self._compile_args)

        self.Traverse(translation_unit.cursor)

    def Error(self, node, msg):
        self.total_errors += 1
        l = node.location
        self.has_error = True
        print("error %s:%d:%d: %s" % (l.file, l.line, l.column, msg))
        print("")
        print(self._filedata[l.line-1].rstrip())
        print("~"*l.column + "^")
        print("")

    def CheckPublicMemberFunction(self, node):
        pass

    def CheckPrivateMemberVariable(self, node):
        pass

    def CheckArgument(self, node):
        pass

    def CheckReturnType(self, node):
        pass

    def CheckClass(self, node):
        pass

    def CheckUsingTypeAlias(self, node):
        pass

    def CheckTypedef(self, node):
        pass

    def Traverse(self, node):
        kind = str(node.kind).split(".")[1]

        if not node.location.file is None:
            if str(node.location.file) != self._filename:

                return
        if kind == "NAMESPACE":
            ns = node.spelling or node.displayname
            if ns == "std":
                return

        if kind == "TYPEDEF_DECL":
            self.CheckTypedef(node)
        elif kind == "TYPE_ALIAS_DECL":
            self.CheckUsingTypeAlias(node)
        elif kind == "FIELD_DECL":
            if node.access_specifier == cldoc.clang.cindex.AccessSpecifier.PRIVATE:
                self.CheckPrivateMemberVariable(node)
            # TODO: Public and protected
        elif kind == "PARM_DECL":
            self.CheckArgument(node)
        elif kind == "CXX_METHOD":
            if node.access_specifier == cldoc.clang.cindex.AccessSpecifier.PUBLIC:
                self.CheckPublicMemberFunction(node)

            self.CheckReturnType(node)
        elif kind == "CLASS_DECL":
            self.CheckClass(node)
        elif kind == "CLASS_TEMPLATE":
            self.CheckClass(node)

        for x in node.get_children():
            self.Traverse(x)
