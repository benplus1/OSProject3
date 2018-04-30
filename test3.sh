#!/bin/bash
ROOT="/tmp/hkc33/mountdir"

mkdir $ROOT/firstdir

echo Creating Folders
for i in {1..10}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	mkdir $path
	touch $path/test.txt 	
done

echo Printing Files
ls $ROOT/firstdir

for i in {1..10}
do
	name=bruh$i
	path=$ROOT/firstdir/$name/test.txt
	
	for i in {1..10}
	do
		echo Wassup >> $path
	done
	cat $path
	echo
done

echo Deleting Folders
for i in {1..10}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	
	rm -r $path
	echo
done
