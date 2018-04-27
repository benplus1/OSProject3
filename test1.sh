#!/bin/bash
ROOT="/tmp/hkc33/mountdir"

mkdir $ROOT/firstdir

echo Creating Files
for i in {1..2}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	echo $path
	touch $path 	
	echo YADADADADA$i >> $path
done

echo Printing Files
ls $ROOT/firstdir

for i in {1..2}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	cat $path
	echo
done

echo Deleting Files
for i in {2..1}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	rm $path
done
ls $ROOT/firstdir
