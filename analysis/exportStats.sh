#!/usr/bin/env bash
config=$1
name="${config}*.vec"
total=$(find results/ -iname $name | wc -w)
i=1
find results/ -iname $name | while read name; do
	mkdir -p "results/JSON"
	out=$(echo $name | sed 's/\.vec//' | sed 's/results/results\/JSON/')
	echo -n "Exporting file $i of $total: $name... "
	scavetool x -f 'module("*")' -o ${out}.json -F JSON $name
	((i++))
done
