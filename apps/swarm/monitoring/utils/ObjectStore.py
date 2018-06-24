# This is a basic object storage class.
# It doesn't care what the hash objects represent, and can be parameterised with lambdas
# for the basic indexing functions.

class ObjectStore(object):

    def deflIdGenFunc(obj, store):
        res = store.nextid
        store.nextid += 1
        return res

    def __init__(self,
                 obj_id_fieldname='id', # This field in the object will be used to store its ID
                 id_gen_func=deflIdGenFunc, # Given an object and the datastore, please return an id for the new object
                 type_coercion_function=None # Given an object, coerce its types if necessary
             ):
        self.datastore = {}
        self.nextid = 1

        self.id_gen_func = id_gen_func
        self.type_coercion_function = type_coercion_function
        self.obj_id_fieldname = obj_id_fieldname

    def add(self, obj):
        i = self.id_gen_func(obj, self)
        if self.obj_id_fieldname:
            obj[self.obj_id_fieldname] = i
        if self.type_coercion_function:
            obj = self.type_coercion_function(obj)
        self.datastore[i] = obj

    def remove(self, ident):
        if ident in self.datastore:
            obj = self.datastore[i]
            del self.datastore[i]
            return obj
        return None

    def find(self, ident):
        if ident in self.datastore:
            return self.datastore[i]
        return None

    def all(self):
        return self.datastore[i].values()

    def iter(self, func):
        for obj in self.datastore[i].values():
            func(obj)
