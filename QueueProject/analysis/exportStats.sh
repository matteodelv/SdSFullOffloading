#!/usr/bin/env bash
find ../results/ -iname '*.vec' | while read name; do
	out=$(echo $name | sed 's/\.vec//' | sed 's/results/results\/JSON/')
	echo Exporting $name...
	#echo out $out
	scavetool x -f 'module("*")' -o ${out}.json -F JSON $name
done
