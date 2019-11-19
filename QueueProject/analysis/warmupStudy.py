#!/usr/bin/env python3

import argparse
import json
import os
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import re
import pprint
from collections import defaultdict
plt.style.use("ggplot")
mpl.rcParams["lines.linewidth"] = 1



def loadData(inputDir):
	selectedFiles = []
	for root, dirs, files in os.walk(inputDir):
		for file in files:
			if "SetupAnalysis" in file and file.endswith(".json"):
				selectedFiles.append(os.path.join(root, file))

	dataDict = {}
	dataKeys = {"seed": [], "policy": [], "renegingTime": []}
	for file in selectedFiles:
		with open(file, "r", encoding="utf-8") as input:
			match = re.search(r"seed=[0-9]+,policy=[1|2],renegingTime=[0-9]+", file)
			if match:
				keys = match.group(0).split(",")
				fileData = json.load(input)
				if len(fileData.keys()) > 1:
					raise Exception("Only one key element expected; found {}".format(len(fileData.keys())))
				outermostKey = list(fileData.keys())[0]
				dataDict.setdefault(keys[2], {}).setdefault(keys[1], {})[keys[0]] = fileData[outermostKey]
				for elem in keys:
					key, delim, value = elem.partition("=")
					if value not in dataKeys[key]:
						dataKeys[key].append(value)

	return dataDict, dataKeys


def runningAvg(x):
	return np.cumsum(x) / np.arange(1, x.size + 1)

def filterData(data, measureKey, policy, renegingTime):
	policyKey = "policy=" + policy
	renegingTimeKey = "renegingTime=" + renegingTime
	relevantData = data[renegingTimeKey][policyKey]
	
	times = []
	values = []
	legends = []
	for seedKey, simData in relevantData.items():
		vectors = simData["vectors"]
		measureVector = [vec for vec in vectors if vec["name"] == measureKey][0]
		measureValues = np.array(measureVector["value"])
		timeValues = np.array(measureVector["time"])
		times.append(timeValues)
		values.append(measureValues)
		legends.append("{}, {}, {}".format(renegingTimeKey, policyKey, seedKey))
	
	return times, values, legends

def quantizeData(timeData, valData):
	maxIndex = np.argmax([len(v) for v in timeData])
	windows = np.arange(start=0.0, stop=timeData[maxIndex][len(timeData[maxIndex])-1], step=10.0)
	
	windowedData = {}
	for i in range(len(timeData)):
		timeVals = timeData[i]
		vals = valData[i]
		ids = np.digitize(timeVals, windows).tolist()
		for id, time, val in zip(ids, timeVals, vals):
			windowedData.setdefault(id, []).append((time, val))
	
	quantizedTimes = []
	quantizedValues = []
	for key in sorted(windowedData.keys()):
		valuesList = windowedData[key]
		times = np.array(list(map(lambda e: e[0], valuesList)))
		values = np.array(list(map(lambda e: e[1], valuesList)))
		quantizedTimes.append(np.mean(times))
		quantizedValues.append(np.mean(values))
	
	return np.array(quantizedTimes), np.array(quantizedValues)
	

def plotMeanServiceTime(data, keys, plotMeanOnly=True):
	maxCommonSize = np.Inf
	timeValues, values, legends = filterData(data, "totalServiceTime:vector", "1", "40")
	for timeData, seedData, label in zip(timeValues, values, legends):
		runAvgd = runningAvg(seedData)
		if runAvgd.size < maxCommonSize:
			maxCommonSize = runAvgd.size
		if not plotMeanOnly:
			plt.plot(timeData, runAvgd, label=label)
	
	qTimes, qValues = quantizeData(timeValues, values)
	
	plt.plot(qTimes, runningAvg(qValues), label="Averaged on different seed runs")
	plt.xlabel("Simulation Time")
	plt.ylabel("Mean Job Service Time")
	plt.legend()
	plt.show()


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", required=True, type=str)
	args = parser.parse_args()
	data, keys = loadData(args.inputDir)
	#pp = pprint.PrettyPrinter(depth=4)
	#pp.pprint(data)
	plotMeanServiceTime(data, keys, plotMeanOnly=False)



