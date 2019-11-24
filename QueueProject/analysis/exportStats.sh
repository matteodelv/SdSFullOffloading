#!/usr/bin/env bash
config=$1
name="${config}*.vec"
find results/ -iname $name | while read name; do
	out=$(echo $name | sed 's/\.vec//' | sed 's/results/results\/JSON/')
	echo Exporting $name...
	#echo out $out
	scavetool x -f 'module("*")' -o ${out}.json -F JSON $name
done
