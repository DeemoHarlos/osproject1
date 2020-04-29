#!/bin/bash
array=(TIME_MEASUREMENT FIFO_1 PSJF_2 RR_3 SJF_4)
for f in ${array[@]} ; do
	echo \[Project 1\]\ running\ ${f}.txt ;
	sudo dmesg -c > /dev/null ;
	newPath=output/${f} ;
	echo \[Project 1\]\ ${f}_stdout.txt: ;
	./main < testset/${f}.txt ;
	echo \[Project 1\]\ ${f}_dmesg.txt: ;
	dmesg | grep Project1 ;
	echo ;
done
