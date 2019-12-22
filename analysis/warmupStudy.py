#!/usr/bin/env python3

import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import os
from utils import loadData, runningAvg, filterData, quantizeData, setupPlots, plotGraph, getTupleValues


def plotTrends(data, keys, outputDir, vector, plotTitle, fileName, moduleName=None):
	print("Plotting Service Times for {}...".format(plotTitle))
	
	plotData = []
	for renTime in keys["renegingTime"]:
		times, values, _ = filterData(data, vector, renTime, moduleName)
		
		assert len(times) == len(values)
		
		seedsData = []
		for seed in keys["seed"]:
			#print("ren time: {}, seed: {}".format(renTime, seed))
			seedsData.append((times[seed], values[seed]))
		
		qTimes, qValues = quantizeData(getTupleValues(0, seedsData), getTupleValues(1, seedsData), step=100.0)
		plotData.append((qTimes, runningAvg(qValues)))
	
	titles = {
		"title": plotTitle,
		"x": "Simulation Time [s]",
		"y": "Service Time [s]"
	}
	plotGraph(getTupleValues(0, plotData), getTupleValues(1, plotData), titles, savePath=os.path.join(outputDir, fileName))
	

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", required=True, type=str)
	parser.add_argument("--outputDir", required=True, type=str)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	os.makedirs(plotsPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir, "SetupAnalysis")
	setupPlots()
	
	vectors = ["jobServiceTime:vector", "wifiActiveTime:vector", "cellActiveTime:vector", "deadlineDistrib:vector", "jobServiceTime:vector"]
	modules = ["FullOffloadingNetwork.cellularQueue", None, None, None, "FullOffloadingNetwork.wifiQueue"]
	fileNames = ["SetupAnalysis_ServiceTime_CellularQueue.png", "SetupAnalysis_WiFiQueue_StateDistribution.png", "SetupAnalysis_CellularQueue_StateDistribution.png", "SetupAnalysis_DeadlineDistributions.png", "SetupAnalysis_ServiceTime_WiFiQueue.png"]
	titles = ["Cellular Queue", "WiFi State Distribution", "Cellular State Distribution", "Deadline Distributions", "WiFi Queue"]
	
	for vec, module, name, title in zip(vectors, modules, fileNames, titles):
		plotTrends(data, keys, plotsPath, vec, title, name, module)


