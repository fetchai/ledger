Configure git<a name="git_configuration"/>
==========================================
It is necessary to configure your local git setup based on [this guide](https://github.com/uvue-git/docker-images/blob/master/README_git_setup.md), otherwise pulling the submodules might not work.
Once you have modified your .gitconfig you should be able to do

git pullall

to get all the submodules required for the project.

Docker - build, develop & run stuff in docker container<a name="docker"/>
=========================================================================
This project has full support for Docker - it is dedicated to be used for building whole project, execute resulting binaries and do whole development using docker, please see details in [this guide](https://github.com/uvue-git/docker-images/blob/master/README.md#guick_usage_guide).

In short, it should be easy to use the docker environment like so:

# Build docker (optional)
sudo develop-image/scripts/docker-build-img.sh

# Run docker
sudo develop-image/scripts/docker-run.sh bash

Initiate
========
git pull  
git pullall

Running on Ubuntu
=========
sudo apt-get install libssl-dev cmake  
sudo apt-get install libpng-dev  
sudo apt-get install python-dev

Build
=====

mkdir build  
cd build  
cmake ..  
make -j8

Generate coverage quick start
=====

cd build
# Configure build in cmake (adds coverage flags)
ccmake .  
	* CMAKE_BUILD_TYPE - set this to Debug  
	* FETCH_ENABLE_COVERAGE - set this to ON  
	* 'c' to configure  
	* 'q' to quit

# Have to remake the executables before generating coverage
make -j8

# Generate coverage
cd ../  
./scripts/generate_coverage.py ./build

# Coverage should now be in build/coverage/...
ls build/coverage

# Optional, view in browser (outside docker)
chromium-browser ./build/coverage

Test
====

cd build  
ctest

Notes
=====

If the changes in the PR do not adhere to Fetch's coding style, the CI will automatically reject it.

To automatically apply trivial stylistic fixes, run (best done in docker due to requiring modern clang tools, clang-format):

./scripts/apply-style.py

To automatically run static analysis to check for and fix coding style violations, run (best done in docker due to requiring modern clang tools, clang-tidy):

./scripts/run-static-analysis.py ./build --fix

Note: this takes a long time, and running with the fix flag may cause clang to break the code (changing
variable names without propagating their updates). Therefore it is recommended you have a clean HEAD so as
to identify the changes that have been made. Build the project again before pushing it as it's likely
to be broken
