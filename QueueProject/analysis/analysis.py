#!/usr/bin/env python3

import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import os
import copy
from utils import filterData, quantizeData, splitJobsByQueue, setupPlots, loadData, runningAvg, plotGraph

import pprint


def extractWiFiCellularData(data, keys, saveDir=None):
	updatedData = copy.deepcopy(data)
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			sTimes, serviceTimeValues, legends = filterData(data, "totalServiceTime:vector", policy, renTime)
			qTimes, queuesVisitedValues, _ = filterData(data, "queuesVisited:vector", policy, renTime)	
	
			wifiTimes = []
			wifiServiceTimes = []
			cellularTimes = []
			cellularServiceTimes = []
	
			for i in range(len(sTimes)):
				wifiJobs, cellularJobs = splitJobsByQueue([sTimes[i], qTimes[i]], serviceTimeValues[i], queuesVisitedValues[i])
		
				wifiSeedTime = wifiJobs[:,:1].reshape(-1).tolist()
				wifiSeedSerTime = wifiJobs[:,1:].reshape(-1).tolist()
				wifiTimes.append(wifiSeedTime)
				wifiServiceTimes.append(wifiSeedSerTime)
		
				cellSeedTime = cellularJobs[:,:1].reshape(-1).tolist()
				cellSeedSerTime = cellularJobs[:,1:].reshape(-1).tolist()
				cellularTimes.append(cellSeedTime)
				cellularServiceTimes.append(cellSeedSerTime)
				
				policyKey = "policy=" + policy
				renTimeKey = "renegingTime=" + renTime
				seedKey = legends[i].split(", ")[2]
				updatedData[renTimeKey][policyKey][seedKey] = {
					"wifi": {"time": wifiSeedTime, "serviceTime": wifiSeedSerTime},
					"cellular": {"time": cellSeedTime, "serviceTime": cellSeedSerTime}
				}
		
			if saveDir:
				qTimesWifi, qValuesWifi = quantizeData(wifiTimes, wifiServiceTimes, step=100.0)
				qTimesCell, qValuesCell = quantizeData(cellularTimes, cellularServiceTimes, step=200.0)
				titles = {
					"title": "policy={} renegingTime={}".format(policy, renTime),
					"x": "Simulation Time",
					"y": "Service Time"
				}
				legends = ["WiFi - Averaged on runs", "Cellular - Averaged on runs"]
				fileName = "WiFi_Cellular_Scatter_{}_{}_averaged.png".format(policy, renTime)
				plotGraph([qTimesWifi, qTimesCell], [qValuesWifi, qValuesCell], titles, legends, scatter=True, savePath=os.path.join(saveDir, fileName))
				
	return updatedData



if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", type=str, required=True)
	parser.add_argument("--outputDir", type=str, required=True)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	os.makedirs(plotsPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir)
	setupPlots()
	wifiCellData = extractWiFiCellularData(data, keys, plotsPath)
	
	#pp = pprint.PrettyPrinter(depth=3)
	#pp.pprint(data)
	#pp.pprint(keys)


