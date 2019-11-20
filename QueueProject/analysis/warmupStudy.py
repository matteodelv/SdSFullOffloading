#!/usr/bin/env python3

import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import os
from utils import loadData, runningAvg, filterData, quantizeData, setupPlots
	

def plotMeanServiceTime(data, keys, outputDir, plotMeanOnly=True):
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			timeValues, values, legends = filterData(data, "totalServiceTime:vector", policy, renTime)
			for timeData, seedData, label in zip(timeValues, values, legends):
				runAvgd = runningAvg(seedData)
				if not plotMeanOnly:
					plt.plot(timeData, runAvgd, label=label)
	
			qTimes, qValues = quantizeData(timeValues, values)
	
			plt.plot(qTimes, runningAvg(qValues), label="Averaged on runs")
			plt.title("policy={} renegingTime={}".format(policy, renTime))
			plt.xlabel("Simulation Time")
			plt.ylabel("Mean Job Service Time")
			plt.legend()
			
			fileName = "ServiceTime_{}_{}{}.png".format(policy, renTime, "_averaged" if plotMeanOnly else "")
			plt.savefig(os.path.join(outputDir, fileName))
			plt.close()


def plotMeanQueueLength(data, keys, moduleName, outputDir, plotMeanOnly=True):
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			timeValues, values, legends = filterData(data, "queueLength:vector", policy, renTime, moduleName)
			for timeData, seedData, label in zip(timeValues, values, legends):
				if not plotMeanOnly:
					plt.plot(timeData, seedData, label=label, drawstyle="steps-post")
			
			if plotMeanOnly:
				qTimes, qValues = quantizeData(timeValues, values, step=200.0)
				plt.plot(qTimes, qValues, label="Averaged on runs", drawstyle="steps-post")
			
			title = "{} policy={} renegingTime={}".format(moduleName.replace("QueueNetwork.", ""), policy, renTime)
			plt.title(title)
			plt.xlabel("Simulation Time")
			plt.ylabel("Queue Length")
			plt.legend()
			
			fileName = "QueueLength_{}_{}_{}{}.png".format(moduleName.replace("QueueNetwork.", ""), policy, renTime, "_averaged" if plotMeanOnly else "")
			plt.savefig(os.path.join(outputDir, fileName))
			plt.close()
			

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", required=True, type=str)
	parser.add_argument("--outputDir", required=True, type=str)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	os.makedirs(plotsPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir)
	setupPlots()
	plotMeanServiceTime(data, keys, plotsPath, plotMeanOnly=True)
	#plotMeanQueueLength(data, keys, "QueueNetwork.cellularQueue", plotsPath, plotMeanOnly=False)
	#plotMeanQueueLength(data, keys, "QueueNetwork.wifiQueue", plotsPath, plotMeanOnly=False)



