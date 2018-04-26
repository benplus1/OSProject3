#!/bin/bash
cd /tmp/bty10/mountdir

for i in {0..10}
	for j in {0..100}
		touch ($i)_($j).c
		rm ($i)_($j).c
		echo "$i $j"

echo "oaisjfoaiwejroaiwejroaiwejroaidfjasdfj"
