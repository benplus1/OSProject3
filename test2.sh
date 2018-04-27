#!/bin/bash
ROOT="/tmp/hkc33/mountdir"

mkdir $ROOT/firstdir

echo Creating Folders
for i in {1..2}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	mkdir $path
	touch $path/test.txt 	
	echo YADADADADA$i >> $path/test.txt
done

echo Printing Files
ls $ROOT/firstdir

for i in {1..2}
do
	name=bruh$i
	path=$ROOT/firstdir/$name/test.txt
	cat $path
	echo
done

echo Deleting Folders
for i in {1..2}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	rm -r $path
done
ls $ROOT/firstdir
