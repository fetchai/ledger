import os
import subprocess

ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))

if ROOT != '':
    os.chdir(ROOT)
output = subprocess.check_output(" ".join(["git", "diff",  "--name-only", "master"]), shell=True)
output = output.decode("utf-8")
libs = [x for x in output.split("\n") if x.startswith("libs/")]
print("\n".join(libs))
