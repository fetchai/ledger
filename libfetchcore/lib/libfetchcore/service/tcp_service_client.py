
from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers

def define_interface(root, cls, selfname):

    cls.add_method('Connect', 'void', [param('std::string', 'host'), param('uint16_t', 'p')])
    cls.add_method('Disconnect', 'void', [])

    cls.add_method('Call', 'fetch::service::Promise', [param('uint16_t', 'protocol'), param('uint16_t', 'function'), param('fetch::byte_array::ByteArray const &', 'args')] )

def build_class(root, mod, namespace):
    name =  "TCPServiceClient"

    cls = mod.add_class(name)

    self_name = namespace + name
    
    cls.add_constructor([])
#    cls.add_constructor([param(self_name+' const &', 'other')])
#    cls.add_constructor([param('std::string const &', 'other')])    

    
    define_interface(root, cls, self_name)

    return name
    
import os
def build(root, mod, includedir):
    mod.add_include('"'+os.path.join(includedir, "service/tcp_service_client.hpp") +'"')

    ns = mod.add_cpp_namespace("service")
    build_class(root, ns, "fetch::service::")
