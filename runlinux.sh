#!/bin/bash

# [ -d build ] || mkdir build
# cd build
# cmake .. -G "Unix Makefiles"

make

LD_LIBRARY_PATH=/usr/local/lib
export LD_LIBRARY_PATH
./game
