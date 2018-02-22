from .. import libfetchcore
from .. import types
import numpy

class Matrix(object):
    matrix_ = None
    type_ = None

    def __init__(self, *args, **kwargs):
        h = 0
        w = 0

        if len(args) == 1:
           h = args[0]
           w = 1
        elif len(args) == 2:
           h = args[0]
           w = args[1]
        else:
           if "height" in kwargs: h = kwargs["height"]
           if "width" in kwargs: w = kwargs["width"]

        type = types.double
        if "type" in kwargs:
            type = kwargs["type"]

        data = None
        if isinstance(h, list):
            data = h
            h = len(data)
            w = 0
            if h != 0:
                w = len(data[0])
                
        elif isinstance(h, numpy.ndarray):
            data = h
            h,w = data.shape

        cls = None
        if type == types.double:
            cls = libfetchcore.fetch.math.linalg.MatrixDouble
        elif type == types.float:
            cls = libfetchcore.fetch.math.linalg.MatrixFloat
        elif type == types.int:
            cls = libfetchcore.fetch.math.linalg.MatrixInt
        else:
            raise BaseException("Unknown type %s" % str(type))
        
        self.type_ = type
        self.matrix_ = cls(h, w)

        if isinstance(data, list):
            for i in range(h):
                for j in range(w):
                    self.matrix_.Set(i,j, data[i][j])
        elif isinstance(data, numpy.ndarray):
            for i in range(h):
                for j in range(w):
                    self.matrix_.Set(i,j, data[i,j])


    def __getitem__(self, i):
        return self.matrix_.At( i )

    def __getitem__(self, index):
        return self.matrix_.At( index[0], index[1] )

    def __setitem__(self,  i,  v):
        self.matrix_.Set( i, v )

    def __setitem__(self, index, v):
        self.matrix_.Set( index[0], index[1], v )


    def Sum(self):
        return self.matrix_.Sum()
        
    def Min(self):
        return self.matrix_.Min()

    def Max(self):
        return self.matrix_.Max()

    def AbsMin(self):
        return self.matrix_.AbsMin()

    def AbsMax(self):
        return self.matrix_.AbsMax()

    def Mean(self):
        return self.matrix_.Mean()

    def Invert(self):
        return self.matrix_.Invert()

    def Apply(self, f):
        pass

    def Dot(self, other, ret = None):
        if ret is None: ret = Matrix(type = self.type_)
        self.matrix_.Dot(other.matrix_, ret.matrix_)
        return ret
    
    def DotTransposedOf(self, other, ret = None):
        if ret is None: ret = Matrix(type = self.type_)
        self.matrix_.DotTransposedOf(other.matrix_, ret.matrix_)
        return ret
        
def AsNPArray(mat):
    ret = numpy.zeros((mat.height(), mat.width()))
    for i in range(mat.height()):
        for j in range(mat.width()):
            ret[i,j] = mat.matrix_.At(i,j)
    return ret

