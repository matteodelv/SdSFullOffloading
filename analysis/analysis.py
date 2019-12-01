#!/usr/bin/env python3

import argparse
import numpy as np
import os
import copy
from utils import filterData, quantizeData, setupPlots, loadData, plotGraph, extractWiFiCellularData, getTupleValues


def computeMeanResponseTime(data, keys, outputDir):
	print("Calculating Mean Response Time... ")
	
	lambdaValue = 0.5
	responseData = copy.deepcopy(data)
	
	mrtData = []
	for renTime in keys["renegingTime"]:
		for policy in keys["policy"]:
			wifiTimes, wifiValues, _ = filterData(data, "queueLength:vector", policy, renTime, "QueueNetwork.wifiQueue")
			cellTimes, cellValues, _ = filterData(data, "queueLength:vector", policy, renTime, "QueueNetwork.cellularQueue")
			remTimes, remValues, _ = filterData(data, "queueLength:vector", policy, renTime, "QueueNetwork.remoteQueue")
			
			qTimes, qVals = quantizeData([wifiTimes[0], cellTimes[0], remTimes[0]], [wifiValues[0], cellValues[0], remValues[0]], step=100.0)
			
			mrt = 1/lambdaValue * np.sum(np.mean(qVals))
			mrtData.append([int(renTime), mrt, int(policy)])
			
			renTimeKey = "renegingTime=" + renTime
			policyKey = "policy=" + policy
			responseData[renTimeKey][policyKey] = mrt
	
	mrtData = np.array(mrtData)
	plotData = []
	for policy in keys["policy"]:
		mask = (mrtData[:,2] == int(policy))
		polData = mrtData[mask]
		polData = polData[polData[:,0].argsort()]
		xs = polData[:,0]
		ys = polData[:,1]
		coeff = np.polyfit(xs.flatten(), ys.flatten(), 7)
		p = np.poly1d(coeff)
		newys = p(xs).tolist()
		plotData.append((xs.tolist(), newys, "policy: {}, trend".format(policy)))
		#plotData.append((xs.tolist(), ys.tolist(), "policy: {}".format(policy)))
		
	
	titles = {
		"title": "Full Offloading Model",
		"x": "Deadline [min]",
		"y": "Mean Response Time [min]"
	}
	filePath = os.path.join(outputDir, "FullOffloading_Response_Deadline.png")
	plotGraph(getTupleValues(0, plotData), getTupleValues(1, plotData), titles, legends=getTupleValues(2, plotData), ylim=(5, 35), savePath=filePath)
	
	return responseData
		

def computeMeanEnergyConsumption(data, keys, outputDir):
	print("Calculating Mean Energy Consumption... ")
	
	wifiPowerCoefficient = 0.7
	cellularPowerCoefficient = 2.5
	lambdaValue = 0.5
	simulationDuration = 2800000
	energyData = copy.deepcopy(data)
	
	mecData = []
	
	for renTime in keys["renegingTime"]:
		renTimeKey = "renegingTime=" + renTime
		
		for policy in keys["policy"]:
			policyKey = "policy=" + policy
			
			wifiSeedsData = []
			cellSeedsData = []
			
			for vals in data[renTimeKey][policyKey].values():
				wTimes = vals["wifi"]["time"]
				wSerTimes = vals["wifi"]["serviceTime"]
				wActiveTimes = vals["wifi"]["activeTime"]
				cTimes = vals["cellular"]["time"]
				cSerTimes = vals["cellular"]["serviceTime"]
				wifiSeedsData.append((wTimes, wSerTimes, wActiveTimes))
				cellSeedsData.append((cTimes, cSerTimes))
			
			wifiTimes, wifiSerTimes = quantizeData(getTupleValues(0, wifiSeedsData), getTupleValues(1, wifiSeedsData), step=1000.0)
			cellTimes, cellSerTimes = quantizeData(getTupleValues(0, cellSeedsData), getTupleValues(1, cellSeedsData), step=1000.0)
			activeTimes = getTupleValues(2, wifiSeedsData)
			
			wifiFraction = np.sum(wifiSerTimes) / np.sum(activeTimes)
			cellFraction = np.sum(cellSerTimes) / simulationDuration
			mec = 1/lambdaValue * np.sum(wifiFraction * wifiPowerCoefficient + cellFraction * cellularPowerCoefficient)
			
			mecData.append([int(renTime), mec, int(policy)])
			energyData[renTimeKey][policyKey] = mec
			
	
	mecData = np.array(mecData)
	plotData = []
	for policy in keys["policy"]:
		mask = (mecData[:,2] == int(policy))
		polData = mecData[mask]
		polData = polData[polData[:,0].argsort()]
		xs = polData[:,0]
		ys = polData[:,1]
		coeff = np.polyfit(xs.flatten(), ys.flatten(), 5)
		p = np.poly1d(coeff)
		newys = p(xs).tolist()
		plotData.append((xs.tolist(), newys, "policy: {}, trend".format(policy)))
		#plotData.append((xs.tolist(), ys.tolist(), "policy: {}".format(policy)))
		
	
	titles = {
		"title": "Full Offloading Model",
		"x": "Deadline [min]",
		"y": "Mean Energy Consumption [J]"
	}
	filePath = os.path.join(outputDir, "FullOffloading_Energy_Deadline.png")
	plotGraph(getTupleValues(0, plotData), getTupleValues(1, plotData), titles, legends=getTupleValues(2, plotData), savePath=filePath)
	
	return energyData


def computeERWP(responseData, energyData, keys, outputDir, w=None):
	print("Calculating ERWP... ")
	
	if not w:
		w = [0.5]
	
	renTimesMap = {"8": 0.125, "10": 0.1, "18": 0.056, "25": 0.04, "33": 0.03, "40": 0.025, "50": 0.02, "65": 0.015, "80": 0.0125, "100": 0.01, "120": 0.008, "140": 0.007, "160": 0.00625, "180": 0.0055, "200": 0.005, "220": 0.0045, "240": 0.004, "260": 0.0038, "280": 0.0035, "300": 0.0033}
	
	erwpData = []
	for exp in w:
		for policy in keys["policy"]:
			policyKey = "policy=" + policy
			
			for renTime in keys["renegingTime"]:
				renTimeKey = "renegingTime=" + renTime
				meanRespTime = responseData[renTimeKey][policyKey]
				meanEnergyCons = energyData[renTimeKey][policyKey]
				erwp = np.power(meanEnergyCons, exp) * np.power(meanRespTime, 1-exp)
				erwpData.append([renTimesMap[renTime], erwp, int(policy), exp])
	
	erwpData = np.array(erwpData)
	
	plotData = []
	for exp in w:
		for policy in keys["policy"]:
			mask = (erwpData[:,3] == exp) & (erwpData[:,2] == int(policy))
			subMatrix = erwpData[mask]
			subMatrix = subMatrix[subMatrix[:,0].argsort()]
			xs = subMatrix[:,0]
			ys = subMatrix[:,1]
			coeff = np.polyfit(xs.flatten(), ys.flatten(), 3)
			p = np.poly1d(coeff)
			newys = p(xs).tolist()
			plotData.append((xs.tolist(), p(xs), "policy: {}, w: {}".format(policy, exp)))
	
	titles = {
		"title": "Full Offloading Model ERWP",
		"x": "Reneging Rate r",
		"y": "ERWP"
	}
	filePath = os.path.join(outputDir, "FullOffloading_ERWP_RenegingRate.png")
	plotGraph(getTupleValues(0, plotData), getTupleValues(1, plotData), titles, getTupleValues(2, plotData), savePath=filePath)
	


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", type=str, required=True)
	parser.add_argument("--outputDir", type=str, required=True)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	os.makedirs(plotsPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir, "BatchExecution")
	setupPlots()
	wifiCellData = extractWiFiCellularData(data, keys, plotsPath)
	responseData = computeMeanResponseTime(data, keys, plotsPath)
	energyData = computeMeanEnergyConsumption(wifiCellData, keys, plotsPath)
	computeERWP(responseData, energyData, keys, plotsPath, w=[0.1, 0.5, 0.9])


