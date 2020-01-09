#!/usr/bin/env python3

import json
import os
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import re
import copy


def setupPlots():
	plt.style.use("ggplot")
	mpl.rcParams["lines.linewidth"] = 1
	mpl.rcParams["grid.color"] = "k"
	mpl.rcParams["grid.linestyle"] = ":"
	mpl.rcParams["grid.linewidth"] = 0.5
	mpl.rcParams["figure.figsize"] = [10.0, 7.0]
	mpl.rcParams["figure.dpi"] = 120
	mpl.rcParams["savefig.dpi"] = 120
	mpl.rcParams["legend.fontsize"] = 9.0


def plotGraph(xs, ys, titles, legends=None, ylim=None, scatter=False, drawStyle="default", savePath=None):
	assert len(xs) == len(ys)
	if legends:
		assert len(xs) == len(legends)

	for i in range(len(xs)):
		if scatter:
			plt.scatter(xs[i], ys[i], label=None if not legends else legends[i], marker="o")
		else:
			plt.plot(xs[i], ys[i], label=None if not legends else legends[i], drawstyle=drawStyle)

	plt.title(titles["title"])
	plt.xlabel(titles["x"])
	plt.ylabel(titles["y"])
	if ylim:
		plt.ylim(ylim[0], ylim[1])
	if legends:
		plt.legend()
	if savePath:
		plt.savefig(savePath)
		plt.close()
	else:
		plt.show()


def getTupleValues(index, data):
	return list(map(lambda e: e[index], data))


def loadData(inputDir, config):
	print("Loading data...")

	selectedFiles = []
	for root, dirs, files in os.walk(inputDir):
		for file in files:
			if config in file and file.endswith(".json"):
				selectedFiles.append(os.path.join(root, file))

	dataDict = {}
	dataKeys = {"seed": [], "renegingTime": []}
	for file in selectedFiles:
		with open(file, "r", encoding="utf-8") as input:
			match = re.search(r"seed=[0-9]+,renegingTime=[0-9]+", file)
			if match:
				keys = match.group(0).split(",")
				fileData = json.load(input)
				if len(fileData.keys()) > 1:
					raise Exception("Only one key element expected; found {}".format(len(fileData.keys())))
				outermostKey = list(fileData.keys())[0]
				_, _, renTimeVal = keys[1].partition("=")
				_, _, seedVal = keys[0].partition("=")
				dataDict.setdefault(renTimeVal, {})[seedVal] = fileData[outermostKey]
				for elem in keys:
					key, delim, value = elem.partition("=")
					if value not in dataKeys[key]:
						dataKeys[key].append(value)

	return dataDict, dataKeys


def runningAvg(x):
	return np.cumsum(x) / np.arange(1, x.size + 1)
	

def filterData(data, measureKey, renegingTime, moduleName=None):
	relevantData = data[renegingTime]
	times = {}
	values = {}
	legends = {}
	for seedKey, simData in relevantData.items():
		legendStr = "{}, {}{}".format(renegingTime, seedKey, "" if moduleName == None else ", {}".format(moduleName.replace("FullOffloadingNetwork.", "")))
		vectors = simData["vectors"]
		measureVector = [vec for vec in vectors if vec["name"] == measureKey and (moduleName == None or vec["module"] == moduleName)]
		if len(measureVector) > 0:
			measureVector = measureVector[0]
		else:
			print("Skipped seed {} for renTime {}...".format(seedKey, renegingTime))
			continue
		measureValues = np.array(measureVector["value"])
		timeValues = np.array(measureVector["time"])
		times[seedKey] = timeValues
		values[seedKey] = measureValues
		legends[seedKey] = legendStr
	
	return times, values, legends


def quantizeData(timeData, valData, step=10.0):
	maxIndex = np.argmax([len(v) for v in timeData])
	windows = np.arange(start=0.0, stop=timeData[maxIndex][len(timeData[maxIndex])-1], step=step)
	
	windowedData = {}
	for i in range(len(timeData)):
		timeVals = timeData[i]
		vals = valData[i]
		ids = np.digitize(timeVals, windows).tolist()
		for id, time, val in zip(ids, timeVals, vals):
			windowedData.setdefault(id, []).append((time, val))
	
	quantizedTimes = []
	quantizedValues = []
	
	count = 0
	for key in sorted(windowedData.keys()):
		count += 1
		if count % 5 == 0:
			continue
		valuesList = windowedData[key]
		times = np.array(list(map(lambda e: e[0], valuesList)))
		values = np.array(list(map(lambda e: e[1], valuesList)))
		quantizedTimes.append(np.mean(times))
		quantizedValues.append(np.mean(values))
	
	return np.array(quantizedTimes), np.array(quantizedValues)


def splitJobsByQueue(timesList, serviceTimes, queuesVals):
	assert len(timesList) == 2
	for i in range(2):
		assert timesList[i].size == serviceTimes.size
		assert timesList[i].size == queuesVals.size
	
	assert np.all((queuesVals == 2) | (queuesVals == 3))
	assert np.array_equal(timesList[0], timesList[1])
	
	timesReshaped = timesList[0].reshape(-1, 1)
	serviceTimesReshaped = serviceTimes.reshape(-1, 1)
	queuesValsReshaped = queuesVals.reshape(-1, 1)
	dataMatrix = np.concatenate((timesReshaped, serviceTimesReshaped, queuesValsReshaped), axis=1)
	
	assert dataMatrix.shape[0] == serviceTimes.size
	
	wifiQueueJobs = dataMatrix[dataMatrix[:,2] == 2]
	cellularQueueJobs = dataMatrix[dataMatrix[:,2] == 3]
	
	assert dataMatrix.size == wifiQueueJobs.size + cellularQueueJobs.size
	
	# dropping queuesVisited row
	wifiQueueJobs = wifiQueueJobs[:,:2]
	cellularQueueJobs = cellularQueueJobs[:,:2]
	
	return wifiQueueJobs, cellularQueueJobs
	

def extractWiFiCellularData(data, keys, saveDir=None):
	print("Extracting WiFi and Cellular data...")
	
	updatedData = copy.deepcopy(data)
	for renTime in keys["renegingTime"]:
		sTimes, serviceTimeValues, legends = filterData(data, "totalServiceTime:vector", renTime)
		qTimes, queuesVisitedValues, _ = filterData(data, "queuesVisited:vector", renTime)	
		waTimes, wifiActiveValues, _ = filterData(data, "wifiActiveTime:vector", renTime)
		caTimes, cellActiveValues, _ = filterData(data, "cellActiveTime:vector", renTime)
		dTimes, dValues, dLegends = filterData(data, "deadlineDistrib:vector", renTime)
	
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
			
			seedKey = legends[i].split(", ")[1]
				
			updatedData[renTime][seedKey] = {
				"deadline": {
					"time": dTimes[i].tolist(),
					"values": dValues[i].tolist(),
					"legends": dLegends[i]
				},
				"wifi": {
					"stTime": wifiSeedTime,
					"serviceTime": wifiSeedSerTime,
					"aTime": waTimes[i].tolist(),
					"activeTime": wifiActiveValues[i].tolist()
				},
				"cellular": {
					"stTime": cellSeedTime,
					"serviceTime": cellSeedSerTime,
					"aTime": caTimes[i].tolist(),
					"activeTime": cellActiveValues[i].tolist()
				}
			}
		
		if saveDir:
			qTimesWifi, qValuesWifi = quantizeData(wifiTimes, wifiServiceTimes, step=100.0)
			qTimesCell, qValuesCell = quantizeData(cellularTimes, cellularServiceTimes, step=200.0)
			titles = {
				"title": "renegingTime={}".format(renTime),
				"x": "Simulation Time",
				"y": "Service Time"
			}
			legends = ["WiFi - Averaged on runs", "Cellular - Averaged on runs"]
			fileName = "WiFi_Cellular_Scatter_{}_averaged.png".format(renTime)
			plotGraph([qTimesWifi, qTimesCell], [qValuesWifi, qValuesCell], titles, legends, scatter=True, savePath=os.path.join(saveDir, fileName))
			
	return updatedData


