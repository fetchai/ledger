rm -f *.db
mkdir -vp build/swarmlog/
./apps/demoswarm/runswarm.py --binary "./pyfetch pychainnode.py" --members 7 --maxpeers 5 --initialpeers 4 --idlespeed 80 --target 15 --logdir "build/swarmlog/"
