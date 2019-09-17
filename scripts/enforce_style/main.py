import cldoc.clang.cindex
from cldoc.clang.cindex import *
from cldoc.clang.cindex import Config, TypeKind, CursorKind, Index
from style_checker import StyleChecker
import os
import sys
import re
import glob

libclangs = [
    '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/libclang.dylib',
    '/Library/Developer/CommandLineTools/usr/lib/libclang.dylib']

found = False
for libclang in libclangs:
    if os.path.exists(libclang):
        cldoc.clang.cindex.Config.set_library_path(os.path.dirname(libclang))
        found = True
        break

if not found:
    print("Could not find clang")
    exit(-1)


class CustomChecker(StyleChecker):
    member_function_style = re.compile(
        r"^(([A-Z][a-z0-9_]*)+|[a-z][a-z0-9_]*)$")
    type_style = re.compile(r"^([A-Z][a-z0-9_]*)+$")
    any = re.compile(r"^.+$")
    operators = ["=", "==", "!=", "()", "^", "^=", "&", "&=", "|", "|=", "<", "<=", ">", ">=", ">>", "<<", "[]",
                 "+", "+=", "-", "-=", "*", "*=", "++", "--", "/", "/=", "->", "<<=", ">>=", "~", "!", "%", "%="]

    allowed_types = ["int", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
                     "int8_t", "int16_t", "int32_t", "int64_t",
                     "char", "bool", "void", "double", "float"]

    incorrect_type_names = []
    incorrect_function_names = []

    def ValidateType(self, type_node):
        name = type_node.spelling
        name = name.replace("&", "").replace(
            "*", "").replace("const", "").replace("typename", "").strip()

        if name in self.allowed_types:
            return True

        if name.startswith("std::"):
            return True

        if name.startswith("asio::"):
            return True

        # Stripping templates
        if name.endswith(">") and "<" in name:
            name, _ = name.split("<", 1)

        # Stripping namespace - must be done
        # after stripping templates
        if "::" in name:
            _, name = name.rsplit("::", 1)

        match = self.type_style.match(name)

        if match is None:
            print("Failed using", name)
            self.incorrect_type_names.append(name)
            return False

        return True

    def CheckPublicMemberFunction(self, node):
        if "operator" in node.spelling:
            op = node.spelling.replace("operator", "")
            if op in self.operators:
                return

        if self.member_function_style.match(node.spelling) is None:
            self.Error(node, "Function name does not follow style guide.")

    def CheckPrivateMemberVariable(self, node):
        if not node.spelling.endswith("_"):
            self.Error(node, "Private member variables must end with '_'")

    def CheckArgument(self, node):
        return

    def CheckReturnType(self, node):
        if not self.ValidateType(node.result_type):
            self.Error(node, "Return type name '" +
                       node.result_type.spelling+"' does not follow style guide.")

    def CheckClass(self, node):
        pass

    def CheckTypedef(self, node):
        self.Error(node, "typedefs not permitted.")

    def CheckUsingTypeAlias(self, node):
        if not self.ValidateType(node):
            self.Error(node, "Type alias '"+node.spelling +
                       "' does not follow style guide.")


if __name__ == "__main__":
    ignores = ["vm/tokeniser.hpp"]

    if len(sys.argv) == 1:
        print(
            "usage", sys.argv[0], "[input directory/file] [c++ include dir1] [c++ include dir2]")
        print("")
        print("example", sys.argv[0], "../../libs/ /usr/local/include")
        print("")
        exit(-1)

    includes = sys.argv[2:]
    for f in glob.glob(os.path.join(sys.argv[1], "*", "include")):
        includes.append(f)

    checker = CustomChecker(includes)

    for root, _, files in os.walk(sys.argv[1]):
        for f in files:
            should_ignore = False
            for ig in ignores:
                if f.endswith(ig):
                    should_ignore = True

            if should_ignore:
                print("Ignoring:", f)
                continue

            if f.endswith(".hpp"):
                filename = os.path.join(root, f)
                checker(filename)

    if checker.has_error:
        print(set(checker.incorrect_type_names))
        exit(-1)
