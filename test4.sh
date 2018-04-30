#!/bin/bash
ROOT="/tmp/hkc33/mountdir"

mkdir $ROOT/firstdir

FILE=$ROOT/test.txt
touch $FILE

input=End
for i in {1..50000}
do 
	input=Wassup$input
done

touch here.txt
echo $input >> $FILE
echo WritingToLog
cat $FILE >> here.txt
rm $FILE

