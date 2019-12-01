#!/bin/bash

if [ -z "$QUEUEINGLIB" ]
then
	echo "\$QUEUEINGLIB variable not set. Exiting..."
	exit 1
fi

nedPath=".:${QUEUEINGLIB}"
libPath="${QUEUEINGLIB}/libqueueinglib.so"
config=$1

if [ "$config" != "SetupAnalysis" ] && [ "$config" != "BatchExecution" ]
then
	echo "Wrong configuration name. Use 'SetupAnalysis' or 'BatchExecution'. Exiting..."
	exit 2
fi

echo "Launching ${config} configuration..."
./SdSFullOffloading -m -n $nedPath -l $libPath omnetpp.ini -u Cmdenv -c $config

echo "Exporting simulation data..."
source analysis/exportStats.sh $config

if [ "$config" == "SetupAnalysis" ]
then
	echo "Studying warmup parameters..."
	analysis/warmupStudy.py --inputDir results --outputDir analysis
else
	echo "Computing simulation analysis..."
	analysis/analysis.py --inputDir results --outputDir analysis
fi

echo "DONE!"
