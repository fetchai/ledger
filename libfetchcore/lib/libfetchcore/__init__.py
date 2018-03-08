import module_register
from . import stdlib, include_files
from .math.linalg import matrix
from .byte_array import byte_array
from .serializer import byte_array_buffer
from .service import tcp_service_client, promise
from . import type

import os

def build(mod, rel_path = ".."):
    if not module_register.Register("__library__"):
        return 

    mod.add_include("<iomanip>")
    mod.add_include("<iostream>")

    includedir = os.path.join(rel_path, "include")

    stdlib.build(mod, includedir)
    
    ns = mod.add_cpp_namespace("fetch")
    type.build(mod,ns,includedir)
    
    matrix.build(mod, ns, includedir)
    byte_array.build(mod, ns, includedir)

    byte_array_buffer.build(mod, ns, includedir)

    promise.build(mod, ns, includedir)        
    tcp_service_client.build(mod, ns, includedir)    
