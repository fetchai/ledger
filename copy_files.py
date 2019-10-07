import glob
import os 

print("SEARCH")
n, m = 0, 0
for f in glob.glob("../oef-mt-core/mt-search/*/src/cpp/*"):
  _, subdir, _, _, name = f.rsplit("/",4)

  if name == "BUILD":
    continue

  if not (name.endswith(".hpp") or name.endswith(".cpp")):
    continue

  if name.endswith(".cpp"):
    proposed_dir = os.path.join("libs", "oef-search", "src", subdir)
    proposed_location = os.path.join("libs", "oef-search", "src", subdir, name)
  else:
    proposed_dir = os.path.join("libs", "oef-search", "include", "oef-search", subdir)
    proposed_location = os.path.join("libs", "oef-search", "include", "oef-search", subdir, name)

  n += 1
  dest = None
  if os.path.exists(os.path.join("libs", "oef-search", "src", subdir, name)):
    dest = os.path.join("libs", "oef-search", "src", subdir, name)
  elif os.path.exists(os.path.join("libs", "oef-search", "include", "oef-search", subdir, name)):
    dest = os.path.join("libs", "oef-search", "include", "oef-search", subdir, name)

  if dest is None:
    m += 1
    print(name, "=>",subdir)
    dest = proposed_location
    os.system("mkdir -p %s"%proposed_dir)
  else:
    assert(proposed_location == dest)    

  assert(dest.startswith(proposed_dir))
  #os.system("cp %s %s" % (f, dest))
print(n, "/", m)

print("")
print("CORE")
n, m = 0, 0
for f in glob.glob("../oef-mt-core/mt-core/*/src/cpp/*"):
  _, subdir, _, _, name = f.rsplit("/",4)

  if name == "BUILD":
    continue

  if not (name.endswith(".hpp") or name.endswith(".cpp")):
    continue


  if name.endswith(".cpp"):
    proposed_dir = os.path.join("libs", "oef-core", "src", subdir)
    proposed_location = os.path.join("libs", "oef-core", "src", subdir, name)
  else:
    proposed_dir = os.path.join("libs", "oef-core", "include", "oef-core", subdir)
    proposed_location = os.path.join("libs", "oef-core", "include", "oef-core", subdir, name)


  n += 1
  dest = None
  if os.path.exists(os.path.join("libs", "oef-core", "src", subdir, name)):
    dest = os.path.join("libs", "oef-core", "src", subdir, name)
  elif os.path.exists(os.path.join("libs", "oef-core", "include", "oef-core", subdir, name)):
    dest = os.path.join("libs", "oef-core", "include", "oef-core", subdir, name)

  if dest is None:
    m += 1
    print(name, " => ", subdir) 
    dest = proposed_location
    os.system("mkdir -p %s"%proposed_dir)    
  else:
    assert(proposed_location == dest)    

  assert(dest.startswith(proposed_dir))
#  os.system("cp %s %s" % (f, dest))

print(n, "/", m)
print("")
print("BASE")

n, m = 0, 0
for f in glob.glob("../oef-mt-core/base/src/cpp/*/*"):
  _, subdir, name = f.rsplit("/",2)

  if name == "BUILD":
    continue

  if not (name.endswith(".hpp") or name.endswith(".cpp")):
    continue

  if name.endswith(".cpp"):
    proposed_dir = os.path.join("libs", "oef-base", "src", subdir)
    proposed_location = os.path.join("libs", "oef-base", "src", subdir, name)
  else:
    proposed_dir = os.path.join("libs", "oef-base", "include", "oef-base", subdir)
    proposed_location = os.path.join("libs", "oef-base", "include", "oef-base", subdir, name)


  n += 1
  dest = None
  if os.path.exists(os.path.join("libs", "oef-base", "src", subdir, name)):
    dest = os.path.join("libs", "oef-base", "src", subdir, name)
  elif os.path.exists(os.path.join("libs", "oef-base", "include", "oef-base", subdir, name)):
    dest = os.path.join("libs", "oef-base", "include", "oef-base", subdir, name)

  if dest is None:
    print(name, " => ", subdir) 
    m += 1
    dest = proposed_location
    os.system("mkdir -p %s"%proposed_dir)
  else:
    assert(proposed_location == dest)

  assert(dest.startswith(proposed_dir))
#  os.system("cp %s %s" % (f, dest))

print(n, "/", m)
