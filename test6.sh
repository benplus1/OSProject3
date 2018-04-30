#!/bin/bash
ROOT="/tmp/hkc33/mountdir"

mkdir $ROOT/firstdir

input=End
for i in {1..1000}
do 
	input=WassupFriends$input
done
for i in {1..128}
do 
	file=bruh$i
	touch $ROOT/firstdir/$file
	echo  $input >> $ROOT/firstdir/$file
done

ls $ROOT/firstdir
