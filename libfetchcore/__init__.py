import module_register
from . import stdlib, include_files
from .math.linalg import matrix
import os

def build(mod, rel_path = ".."):
    if not module_register.Register("__library__"):
        return 

    mod.add_include("<iomanip>")
    mod.add_include("<iostream>")

    includedir = os.path.join(rel_path, "include")

    stdlib.build(mod, includedir)

    ns = mod.add_cpp_namespace("fetch")
    matrix.build(mod, ns, includedir)
