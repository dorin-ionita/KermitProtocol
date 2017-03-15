#!/bin/bash

SPEED=10
DELAY=10
LOSS=5
CORRUPT=20
FILES=(file1.txt file2.txt)

killall link
killall recv
killall send

./link_emulator/link speed=$SPEED delay=$DELAY loss=$LOSS corrupt=$CORRUPT &> /dev/null &
sleep 1
./kreceiver &
sleep 1

./ksender "${FILES[@]}"

echo "==========================="
for file in "${FILES[@]}"
do
	DIFF=$(diff $file recv_$file) 
	if [ "$DIFF" != "" ] 
	then
		echo "Files $file and recv_$file are not the same!"
	fi
done
echo "==========================="