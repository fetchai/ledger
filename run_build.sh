#!/bin/bash

#make -C build_vim network_gtest -j > vim_build_out.txt 2>&1
make -C build_vim -j > vim_build_out.txt 2>&1

# Note: this will blast the variable
#echo "$?"

if [ $? -eq 0 ]
then
    echo "all OK"
    echo ""
    #cat vim_build_out.txt
else
    echo "all not OK"
    cat vim_build_out.txt
fi

~/repos/scripts/alert_focus
