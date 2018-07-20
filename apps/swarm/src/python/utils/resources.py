import os, sys
from pkgutil import get_loader

# https://stackoverflow.com/questions/5003755/how-to-use-pkgutils-get-data-with-csv-reader-in-python

main_package = None

def initialise(package):
    global main_package
    main_package = package

def textfile(resourceName, as_string=False):
    return resource(main_package, resourceName).decode("utf-8")

def textlines(resourceName, as_string=False):
    return resource(main_package, resourceName).decode("utf-8").rstrip("\n").split("\n")

def binaryfile(package, resourceName, as_string=False):
    return resource(main_package, resourceName)

def resource_list(package):
    loader = get_loader(package)
    print(loader.dir)

def resource(package, resourceName, as_string=False):
    print("R",package, resourceName)
    loader = get_loader(package)
    mod = sys.modules.get(package) or loader.load_module(package)
    if mod != None and hasattr(mod, '__file__'):
        # Modify the resourceName name to be compatible with the loader.get_data
        # signature - an os.path format "filename" starting with the dirname of
        # the package's __file__
        parts = resourceName.split('/')
        resource_name = os.path.join(*parts)
        parts.insert(0, os.path.dirname(mod.__file__))
        full_resource_name = os.path.join(*parts)
    if loader != None:
        if hasattr(loader, 'get_data'):
            if as_string:
                return resource_name
            else:
                try:
                    return loader.get_data(full_resource_name)
                except OSError as ex:
                    if ex.errno == 0:
                        if os.path.exists(resource_name):
                            with open(resource_name, "rb") as binary_file:
                                return binary_file.read()
                    raise
    return None
