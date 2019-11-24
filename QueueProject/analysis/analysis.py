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


def computeMeanResponseTime(data, keys):
	lambdaValue = 0.5
	responseData = copy.deepcopy(data)
	
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			wifiTimes, wifiValues, _ = filterData(data, "queueLength:vector", policy, renTime, "QueueNetwork.wifiQueue")
			cellTimes, cellValues, _ = filterData(data, "queueLength:vector", policy, renTime, "QueueNetwork.cellularQueue")
			remTimes, remValues, _ = filterData(data, "queueLength:vector", policy, renTime, "QueueNetwork.remoteQueue")
			responses = []
			wifiMeans = []
			for i in range(len(wifiValues)):
				wifiMeans.append(np.mean(wifiValues[i]))
				print("WIFI - policy: {}, renTime: {}, len: {}, mean: {}".format(policy, renTime, len(wifiValues[i]), np.mean(wifiValues[i])))
			print("WIFI - policy: {}, renTime: {}, AVERAGED MEAN: {}".format(policy, renTime, np.mean(wifiMeans)))
			responses.append(np.mean(wifiMeans))
			print()
			
			cellMeans = []
			for i in range(len(cellValues)):
				cellMeans.append(np.mean(cellValues[i]))
				print("CELLULAR - policy: {}, renTime: {}, len: {}, mean: {}".format(policy, renTime, len(cellValues[i]), np.mean(cellValues[i])))
			print("CELLULAR - policy: {}, renTime: {}, AVERAGED MEAN: {}".format(policy, renTime, np.mean(cellMeans)))
			responses.append(np.mean(cellMeans))
			print()
			
			remMeans = []
			for i in range(len(remValues)):
				remMeans.append(np.mean(remValues[i]))
				print("REMOTE - policy: {}, renTime: {}, len: {}, mean: {}".format(policy, renTime, len(remValues[i]), np.mean(remValues[i])))
			print("REMOTE - policy: {}, renTime: {}, AVERAGED MEAN: {}".format(policy, renTime, np.mean(remMeans)))
			responses.append(np.mean(remMeans))
			mrt = 1/lambdaValue * np.sum(responses)
			print("MEAN RESPONSE TIME: {} min".format(mrt))
			print()
			print("----------")
			print()
			renTimeKey = "renegingTime=" + renTime
			policyKey = "policy=" + policy
			responseData[renTimeKey][policyKey] = mrt
	return responseData
			
			
			
def computeMeanEnergyConsumption(data, keys):
	wifiPowerCoefficient = 0.7
	cellularPowerCoefficient = 2.5
	lambdaValue = 0.5
	energyData = copy.deepcopy(data)
	
	for policy in keys["policy"]:
		for renTime in keys["renegingTime"]:
			policyKey = "policy=" + policy
			renTimeKey = "renegingTime=" + renTime
			
			wifiAveraged = []
			cellAveraged = []
			for seedKey, seedValues in data[renTimeKey][policyKey].items():
				wifiSerTimes = seedValues["wifi"]["serviceTime"]
				cellSerTimes = seedValues["cellular"]["serviceTime"]
				print("policy: {}, renTime: {}, {}, wifi sum: {:.4f}, cell sum: {:.4f}, wifi mean: {:.4f}, cell mean: {:.4f}".format(policy, renTime, seedKey, np.sum(wifiSerTimes), np.sum(cellSerTimes), np.mean(wifiSerTimes), np.mean(cellSerTimes)))
				wifiAveraged.append(np.sum(wifiSerTimes))
				cellAveraged.append(np.sum(cellSerTimes))
			
			print("policy: {}, renTime: {}, wifi avg sum: {:.4f}, cell avg sum: {:.4f}".format(policy, renTime, np.mean(wifiAveraged), np.mean(cellAveraged)))
			print("policy: {}, renTime: {}, wifi energy: {:.4f} Joule, cell energy {:.4f} Joule".format(policy, renTime, np.mean(wifiAveraged) * wifiPowerCoefficient, np.mean(cellAveraged) * cellularPowerCoefficient))
			sumValue = np.mean(wifiAveraged) * wifiPowerCoefficient + np.mean(cellAveraged) * cellularPowerCoefficient
			mec = 1/lambdaValue * np.sum(sumValue)
			energyData[renTimeKey][policyKey] = mec
			print("policy: {}, renTime: {}, Mean Energy Consumption: {} Joule".format(policy, renTime, mec))
			print()
	
	return energyData


def computeERWP(responseData, energyData, keys, w=None):
	if not w:
		w = [0.5]
	
	for policy in keys["policy"]:
		policyKey = "policy=" + policy
		for renTime in keys["renegingTime"]:
			renTimeKey = "renegingTime=" + renTime
			meanRespTime = responseData[renTimeKey][policyKey]
			meanEnergyCons = energyData[renTimeKey][policyKey]
			
			for exp in w:
				print("policy: {}, renTime: {} - ERWP with w={}: {}".format(policy, renTime, exp, np.power(meanRespTime, exp) * np.power(meanEnergyCons, 1-exp)))
		print()
	


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
	responseData = computeMeanResponseTime(data, keys)
	energyData = computeMeanEnergyConsumption(wifiCellData, keys)
	computeERWP(responseData, energyData, keys, w=[0.1, 0.5, 0.9])
	
	#pp = pprint.PrettyPrinter(depth=3)
	#pp.pprint(data)
	#pp.pprint(keys)


