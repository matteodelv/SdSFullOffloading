#!/usr/bin/env python3

import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import os
from utils import filterData, quantizeData, splitJobsByQueue, setupPlots, loadData

import pprint


def prepareData(data, keys):
	sTimes, serviceTimeValues, _ = filterData(data, "totalServiceTime:vector", "1", "40")
	qTimes, queuesVisitedValues, _ = filterData(data, "queuesVisited:vector", "1", "40")
	
	wifiTimes = []
	wifiServiceTimes = []
	cellularTimes = []
	cellularServiceTimes = []
	for i in range(len(sTimes)):
		wifiJobs, cellularJobs = splitJobsByQueue([sTimes[i], qTimes[i]], serviceTimeValues[i], queuesVisitedValues[i])
		wifiTimes.append(wifiJobs[:,:1].reshape(-1).tolist())
		wifiServiceTimes.append(wifiJobs[:,1:].reshape(-1).tolist())
		
		cellularTimes.append(cellularJobs[:,:1].reshape(-1).tolist())
		cellularServiceTimes.append(cellularJobs[:,1:].reshape(-1).tolist())
		print("Done for i =", i)
	
	qTimesWifi, qValuesWifi = quantizeData(wifiTimes, wifiServiceTimes, step=100.0)
	qTimesCell, qValuesCell = quantizeData(cellularTimes, cellularServiceTimes, step=100.0)
	plt.plot(qTimesWifi, qValuesWifi, label="WiFi - Averaged on runs")
	plt.plot(qTimesCell, qValuesCell, label="Cellular - Averaged on runs")
	plt.legend()
	plt.show()
	


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", type=str, required=True)
	parser.add_argument("--outputDir", type=str, required=True)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	os.makedirs(plotsPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir)
	setupPlots()
	prepareData(data, keys)
	
	#pp = pprint.PrettyPrinter(depth=3)
	#pp.pprint(data)
	#pp.pprint(keys)


