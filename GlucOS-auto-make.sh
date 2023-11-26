#!/bin/bash

Multithreading=1
Timer_interval_ms=10
NCPU=2
NPF=200			# Number of page frames
#NPF=51200
NPSWAP=512		# Number of swap pages
DEBUG=1
User_seg_max="0x800000"	# 8 MiB
#User_seg_max="0x80000000"	# 2 GiB

VIEW=""
for opt in $@
do
	if [ $opt = "clean" ]
	then
		for file in `ls ./build`
		do
			if [ ! $file = "createimage" ]
			then
				rm ./build/$file
			fi
		done
	elif [ $opt = "view" ]
	then
		VIEW="-n"
	elif [ $opt = "nodebug" ]
	then
		DEBUG=0
	fi
done

make all DEBUG=$DEBUG NCPU=$NCPU TINTERVAL=$Timer_interval_ms MTHREAD=$Multithreading NPSWAP=$NPSWAP CFOTHER="-DNPF=$NPF -DUSEG_MAX=$User_seg_max" $VIEW

for opt in $*
do
	if [ $opt = "swap" ]
	then
		make swap NPSWAP=$NPSWAP
	fi
done
