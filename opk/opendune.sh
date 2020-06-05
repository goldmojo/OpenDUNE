#!/bin/sh

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
cd `dirname $0`

#mkdir -p /usr/local/share/timidity
#cp ./timidity.cfg /usr/local/share/timidity

#./timidity -iA &
./opendune
#killall timidity
