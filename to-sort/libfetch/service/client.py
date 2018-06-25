from .. import libfetchcore
import ctypes
types = libfetchcore.fetch.base_types
TCPServiceClient = libfetchcore.fetch.service. TCPServiceClient
Serializer = libfetchcore.fetch.serializers.TypedByte_ArrayBuffer

class Client(object):
    packers_ = {        
        'i8': types.Int8,
        'i16': types.Int16,
        'i32': types.Int32,
        'i64': types.Int64,
        'u8': types.Uint8,
        'u16': types.Uint16,
        'u32': types.Uint32,
        'u64': types.Uint64        
    }
    
    typemapper_ = {
        
    }
    
    def __init__(self, *args, **kwargs):
        self.client_ = TCPServiceClient()

    def Connect(self, host, port):
        self.client_.Connect( host, port )

    def Disconnect(self):
        self.client_.Disconnect()

    def Call(self, protocol, function, *args):
        ser = Serializer()

        for v in args:
            if isinstance(v, tuple):
                t,d = v
                p = Client.packers_[t]            
                ser.Pack( p(d).value() )
            elif v in Client.type_mapper_:
                ## Yet to be implemented
                pass
            else:
                ser.Pack( v )
                
        return self.client_.Call(protocol, function, ser.data())
    
        
