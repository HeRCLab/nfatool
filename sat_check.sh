#!/bin/bash

if [ $# -ne 2 ]; then
	printf "usage: %s <out-file> <cnf-file>\n" $0 > 2
fi

OUT_FILE=$1
CNF_FILE=$2

first=1
pattern1="("
pattern2="("
while read line; do
	for literal in $line; do
		if [ $first -eq "1" ]; then
			pattern1="${pattern1}-${literal}[[:space:]]"
			pattern2="${pattern2}[[:space:]]${literal}[[:space:]]"
			first=0
		else
			pattern1="${pattern1}|-${literal}[[:space:]]"
			pattern2="${pattern2}|[[:space:]]${literal}[[:space:]]"
		fi
	done
done < $OUT_FILE

pattern1="${pattern1})"
pattern2="${pattern2})"

sed -r -i "s/${pattern1}/F /g" $CNF_FILE
sed -r -i "s/${pattern2}/ T /g" $CNF_FILE

sed -r -i "s/\-[0-9]+/T/g" $CNF_FILE
sed -r -i "s/[0-9]+/F/g" $CNF_FILE

ln=1
while read line; do
	if [ ${line:0:1} != "c" ] && [ ${line:0:1} != "r" ]; then
		outcome="F"
		for literal in $line; do
			if [ $literal == "T" ]; then
				outcome="T"
			fi
		done
		
		if [ $outcome == "F" ]; then
			printf "ERROR:  line $ln contains all FALSES\n"
			exit
		fi
	fi
	
	let ln=$ln+1
done < $CNF_FILE
