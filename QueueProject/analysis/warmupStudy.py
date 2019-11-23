#!/usr/bin/env python3

import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import os
from utils import loadData, runningAvg, filterData, quantizeData, setupPlots, plotGraph
	

def plotMeanServiceTime(data, keys, outputDir, plotMeanOnly=True):
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			timeValues, values, legends = filterData(data, "totalServiceTime:vector", policy, renTime)
			
			qTimes, qValues = quantizeData(timeValues, values)
			
			if not plotMeanOnly:
				timeValues.append(qTimes)
				values.append(qValues)
				legends.append("Averaged on runs")
				values = list(map(lambda l: runningAvg(l), values))
			else:
				timeValues = [qTimes]
				values = [runningAvg(qValues)]
				legends = ["Averaged on runs"]
				
			titles = {
				"title": "policy={} renegingTime={}".format(policy, renTime),
				"x": "Simulation Time",
				"y": "Mean Job Service Time"
			}
			fileName = "ServiceTime_{}_{}{}.png".format(policy, renTime, "_averaged" if plotMeanOnly else "")
			plotGraph(timeValues, values, titles, legends, savePath=os.path.join(outputDir, fileName))


def plotMeanQueueLength(data, keys, moduleName, outputDir, plotMeanOnly=True):
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			timeValues, values, legends = filterData(data, "queueLength:vector", policy, renTime, moduleName)
			
			if plotMeanOnly:
				qTimes, qValues = quantizeData(timeValues, values, step=200.0)
				timeValues = [qTimes]
				values = [qValues]
				legends = ["Averaged on runs"]
			
			titles = {
				"title": "{} policy={} renegingTime={}".format(moduleName.replace("QueueNetwork.", ""), policy, renTime),
				"x": "Simulation Time",
				"y": "Queue Length"
			}
			
			fileName = "QueueLength_{}_{}_{}{}.png".format(moduleName.replace("QueueNetwork.", ""), policy, renTime, "_averaged" if plotMeanOnly else "")
			plotGraph(timeValues, values, titles, legends, drawStyle="steps-post", savePath=os.path.join(outputDir, fileName))
			

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", required=True, type=str)
	parser.add_argument("--outputDir", required=True, type=str)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	os.makedirs(plotsPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir)
	setupPlots()
	#plotMeanServiceTime(data, keys, plotsPath, plotMeanOnly=False)
	#plotMeanQueueLength(data, keys, "QueueNetwork.cellularQueue", plotsPath, plotMeanOnly=False)
	plotMeanQueueLength(data, keys, "QueueNetwork.wifiQueue", plotsPath, plotMeanOnly=True)



