
from pybindgen import Module, FileCodeSink, param, retval, cppclass, typehandlers

def define_interface(root, cls, selfname,  datatype):

    cls.add_method('Address', 'std::string', [])
    
#    cls.add_method('Connect', 'void', [param('std::string', 'host'), param('std::string', 'port')])
#    cls.add_method('Connect', 'void', [param('std::string', 'host'), param('uint16_t', 'port')])

    
#    cls.add_method('Reshape', 'void', [param('uint64_t', 'n'), param('uint64_t', 'm')])
#    cls.add_method('Flatten', 'void', [  ] )

def build_class(root, mod, namespace, datatype):
    name =  "TCPClient"
    customname = name + datatype.capitalize().replace("_t", "")

    cls = mod.add_class(name, template_parameters=[datatype], custom_name=customname)

    self_name = namespace + name+"< %s >" % datatype
    cls.add_constructor([])
    cls.add_constructor([param(self_name+' const &', 'other')])
    cls.add_constructor([param('std::size_t', 'm'), param('std::size_t', 'n')])

    define_interface(root, cls, self_name,  datatype)

    return name
    
