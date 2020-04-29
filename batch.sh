#!/bin/bash
for f in testset/* ; do
	sudo dmesg -c > /dev/null ;
	echo ${f#testset/}
	newPath=output/${f#testset/} ;
	./main < $f > ${newPath%.txt}_stdout.txt ;
	dmesg | grep Project1 > ${newPath%.txt}_dmesg.txt ;
done
