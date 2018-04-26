#!/bin/bash
cd ../
make clean
make
fusermount -u /tmp/hkc33/mountdir
cd ./src/
./sfs  /.freespace/hkc/testfsfile /tmp/hkc33/mountdir
ps -ef | grep sfs
