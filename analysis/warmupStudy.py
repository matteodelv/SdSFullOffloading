#!/usr/bin/env python3

import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import os
from utils import loadData, runningAvg, filterData, quantizeData, setupPlots, plotGraph, extractWiFiCellularData, getTupleValues
	

def plotMeanServiceTime(data, keys, outputDir, config="SetupAnalysis"):
	print("Calculating Mean Service Time...")
	
	wifiPlotData = []
	cellPlotData = []
	
	for policy in keys["policy"]:
		policyKey = "policy=" + policy
		
		for renTime in keys["renegingTime"]:
			renTimeKey = "renegingTime=" + renTime
			curConfigData = data[renTimeKey][policyKey]
			
			wifiSeedsData = []
			cellSeedsData = []
			
			for seed in keys["seed"]:
				seedKey = "seed=" + seed
				wifiTimes = curConfigData[seedKey]["wifi"]["time"]
				wifiServiceTimes = curConfigData[seedKey]["wifi"]["serviceTime"]
				cellTimes = curConfigData[seedKey]["cellular"]["time"]
				cellServiceTimes = curConfigData[seedKey]["cellular"]["serviceTime"]
				wifiSeedsData.append((wifiTimes, wifiServiceTimes))
				cellSeedsData.append((cellTimes, cellServiceTimes))
			
			wifiTimes, wifiServiceTimes = quantizeData(getTupleValues(0, wifiSeedsData), getTupleValues(1, wifiSeedsData), step=100.0)
			cellTimes, cellServiceTimes = quantizeData(getTupleValues(0, cellSeedsData), getTupleValues(1, cellSeedsData), step=100.0)
			
			wifiPlotData.append((wifiTimes, runningAvg(wifiServiceTimes)))
			cellPlotData.append((cellTimes, runningAvg(cellServiceTimes)))
	
	titles = {
		"title": "WiFi Queue",
		"x": "Simulation Time",
		"y": "Mean Service Time [s]"
	}
	
	fileName = "{}_ServiceTime_WiFiQueue.png".format(config)
	plotGraph(getTupleValues(0, wifiPlotData), getTupleValues(1, wifiPlotData), titles, savePath=os.path.join(outputDir, fileName))
	
	titles["title"] = "Cellular Queue"
	fileName = "{}_ServiceTime_CellularQueue.png".format(config)
	plotGraph(getTupleValues(0, cellPlotData), getTupleValues(1, cellPlotData), titles, savePath=os.path.join(outputDir, fileName))


def plotMeanQueueLength(data, keys, moduleName, outputDir, plotMeanOnly=True):
	print("Calculating Mean Queue Length for {}...".format(moduleName))
	
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			timeValues, values, legends = filterData(data, "queueLength:vector", policy, renTime, moduleName)
			
			if plotMeanOnly:
				qTimes, qValues = quantizeData(timeValues, values, step=200.0)
				timeValues = [qTimes]
				values = [qValues]
				legends = ["Averaged on runs"]
				
			#values = list(map(lambda e: runningAvg(e), values)) # TODO: check this
			
			titles = {
				"title": "{} policy={} renegingTime={}".format(moduleName.replace("FullOffloadingNetwork.", ""), policy, renTime),
				"x": "Simulation Time",
				"y": "Queue Length"
			}
			
			fileName = "QueueLength_{}_{}_{}{}.png".format(moduleName.replace("FullOffloadingNetwork.", ""), policy, renTime, "_averaged" if plotMeanOnly else "")
			plotGraph(timeValues, values, titles, legends, drawStyle="steps-post", savePath=os.path.join(outputDir, fileName))



if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", required=True, type=str)
	parser.add_argument("--outputDir", required=True, type=str)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	os.makedirs(plotsPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir, "SetupAnalysis")
	wifiCellData = extractWiFiCellularData(data, keys)
	setupPlots()
	plotMeanServiceTime(wifiCellData, keys, plotsPath)
	plotMeanQueueLength(data, keys, "FullOffloadingNetwork.cellularQueue", plotsPath, plotMeanOnly=False)
	plotMeanQueueLength(data, keys, "FullOffloadingNetwork.wifiQueue", plotsPath, plotMeanOnly=False)



