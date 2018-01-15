from ..libaicore import *

class Image(object):
    image_ = None

    def __init__(self, *args, **kwargs):
        h = 0
        w = 0

        filename = None
        if len(args) == 1:
           filename = args[0]
        elif len(args) == 2:
           h = args[0]
           w = args[1]
        else:
           if "height" in kwargs: h = kwargs["height"]
           if "width" in kwargs: w = kwargs["width"]

        if filename is None:
           self.image_ = ImageRGBA(h, w)
        else:
           self.image_ = ImageRGBA()
           self.image_.Load(filename)

    def Load(self, filename):
        self.image_.Load(filename)

    def __getitem__(self, i):
        return self.image_.At( i )

    def __getitem__(self, index):
        return self.image_.At( index[0], index[1] )

    def __setitem__(self,  i,  v):
        self.image_.Set( i, v )

    def __setitem__(self, index, v):
        self.image_.Set( index[0], index[1], v )

    def Min(self):
        return self.image_.Min()

    def Max(self):
        return self.image_.Max()

    def AbsMin(self):
        return self.image_.AbsMin()

    def AbsMax(self):
        return self.image_.AbsMax()

    def Mean(self):
        return self.image_.Mean()
    
    def as_np_array(self):
        ret = numpy.zeros((self.height(), self.width(), 3))
        for i in range(self.height()):
            for j in range(self.width()):
                val = self.image_.At(i,j)
                for k in range(3):
                    ret[i,j, k] = val & 255
                    val >>= 8

        return ret / 255.
