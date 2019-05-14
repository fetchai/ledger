import glob
import os


for root, dirs, _ in os.walk("include"):
    for d in dirs:
        pyincludedir = os.path.join("py" + root, d)
        if not os.path.isdir(pyincludedir):
            print "Making", pyincludedir
            os.mkdir(pyincludedir)
        initfile = os.path.join(pyincludedir, "__init__.pxd")
        if not os.path.isfile(initfile):
            print "Touching", initfile
            with open(initfile, "w") as fb:
                fb.write("\n")
