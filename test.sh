#!/bin/bash
ROOT="/tmp/bty10/mountdir"

mkdir $ROOT/firstdir

echo Creating Folders
for i in {1..128}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	mkdir $path
	touch $path/test.txt
	for j in {1..400}
	do 	
		echo YADADADADA$i$j >> $path/test.txt
	done
done

echo Printing Files
ls $ROOT/firstdir

for i in {1..128}
do
	name=bruh$i
	path=$ROOT/firstdir/$name/test.txt
	cat $path
	echo
done

echo Deleting Folders
for i in {1..128}
do
	name=bruh$i
	path=$ROOT/firstdir/$name
	rm -r $path
done
ls $ROOT/firstdir
