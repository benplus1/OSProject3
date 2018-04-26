#!/bin/bash
cd ../
make clean
make
fusermount -u /tmp/bty10/mountdir
cd ./src/
./sfs  /.freespace/bty10/testfsfile /tmp/bty10/mountdir
ps -ef | grep sfs
