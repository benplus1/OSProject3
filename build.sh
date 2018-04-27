#!/bin/bash
cd ../
make clean
make
fusermount -u /tmp/hkc33/mountdir
rm /.freespace/hkc33/testfsfile
touch /.freespace/hkc33/testfsfile
cd ./src/
./sfs   /.freespace/hkc33/testfsfile /tmp/hkc33/mountdir
ps -ef | grep sfs
