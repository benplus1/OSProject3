#!/bin/bash
ROOT="/tmp/hkc33/mountdir"

mkdir $ROOT/firstdir


for i in {1..100}
do 
	folder=juh$i
	mkdir $ROOT/firstdir/$folder
done

ls $ROOT/firstdir
rm -r $ROOT/firstdir
