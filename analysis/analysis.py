#!/usr/bin/env python3

import argparse
import numpy as np
import os
import copy
import scipy.stats as stats
from utils import filterData, setupPlots, loadData, plotGraph, getTupleValues


def computeMeanResponseTime(data, keys, outputDir):
	print("Plotting Mean Response Time...")
	
	responseData = copy.deepcopy(data)
	mrtData = []
	confidenceValues = {}
	
	for renTime in keys["renegingTime"]:
		times, values, _ = filterData(data, "totalResponseTime:vector", renTime)
		confidenceValues[renTime] = {}
			
		respTimes = []
		for seed in keys["seed"]:
			meanRespTimePerSeed = np.mean(values[seed]) / 60.0
			respTimes.append(meanRespTimePerSeed)
			confidenceValues[renTime][seed] = meanRespTimePerSeed
			
		meanRespTimePerDeadline = np.mean(respTimes)
		#print("ren time: {}, MRT: {:.4f}".format(renTime, meanRespTimePerDeadline))
			
		mrtData.append([int(renTime), meanRespTimePerDeadline])
		responseData[renTime] = meanRespTimePerDeadline
	
	mrtData = np.array(mrtData)
	plotData = []
	
	mrtData = mrtData[mrtData[:,0].argsort()]
	xs = mrtData[:,0] / 60.0
	ys = mrtData[:,1]
	coeff = np.polyfit(xs.flatten(), ys.flatten(), 7)
	p = np.poly1d(coeff)
	newys = p(xs).tolist()
	plotData.append((xs.tolist(), newys, "trend"))
	#plotData.append((xs.tolist(), ys.tolist(), "values"))
	
	titles = {
		"title": "Full Offloading Model",
		"x": "Deadline [min]",
		"y": "Mean Response Time [min]"
	}
	filePath = os.path.join(outputDir, "FullOffloading_Response_Deadline.png")
	plotGraph(getTupleValues(0, plotData), getTupleValues(1, plotData), titles, legends=getTupleValues(2, plotData), savePath=filePath)

	return responseData, confidenceValues


def computeMeanEnergyConsumption(data, keys, outputDir):
	print("Plotting Mean Energy Consumption... ")
	
	wifiPowerCoefficient = 0.7
	cellularPowerCoefficient = 2.5
	energyData = copy.deepcopy(data)
	
	mecData = []
	confidenceValues = {}
	
	for renTime in keys["renegingTime"]:
		wTimes, wValues, _ = filterData(data, "jobServiceTime:vector", renTime, "FullOffloadingNetwork.wifiQueue")
		cTimes, cValues, _ = filterData(data, "jobServiceTime:vector", renTime, "FullOffloadingNetwork.cellularQueue")
		
		confidenceValues[renTime] = {}
		mecs = []
		for seed in keys["seed"]:
			if seed not in wValues or seed not in cValues:
				continue
			wst = np.sum(wValues[seed]) * wifiPowerCoefficient
			cst = np.sum(cValues[seed]) * cellularPowerCoefficient
			mec = (np.sum([wst, cst]) / (len(wValues[seed]) + len(cValues[seed]))) / 60.0
			
			mecs.append(mec)
			confidenceValues[renTime][seed] = mec
			
		mec = np.mean(mecs)
		#print("ren time: {}, MEC: {:.4f}".format(renTime, mec))
		
		mecData.append([int(renTime), mec])
		energyData[renTime] = mec
	
	mecData = np.array(mecData)
	plotData = []
		
	mecData = mecData[mecData[:,0].argsort()]
	xs = mecData[:,0] / 60.0
	ys = mecData[:,1]
	coeff = np.polyfit(xs.flatten(), ys.flatten(), 5)
	p = np.poly1d(coeff)
	newys = p(xs).tolist()
	plotData.append((xs.tolist(), newys, "trend"))
	#plotData.append((xs.tolist(), ys.tolist(), "values"))	
	
	titles = {
		"title": "Full Offloading Model",
		"x": "Deadline [min]",
		"y": "Mean Energy Consumption [J]"
	}
	filePath = os.path.join(outputDir, "FullOffloading_Energy_Deadline.png")
	plotGraph(getTupleValues(0, plotData), getTupleValues(1, plotData), titles, legends=getTupleValues(2, plotData), savePath=filePath)
	
	return energyData, confidenceValues


def computeERWP(responseData, energyData, keys, outputDir, w=None):
	print("Plotting ERWP... ")
	
	if not w:
		w = [0.5]
	
	erwpData = []
	for exp in w:
		for renTime in keys["renegingTime"]:
			meanRespTime = responseData[renTime]
			meanEnergyCons = energyData[renTime]
			erwp = np.multiply(np.power(meanEnergyCons, exp), np.power(meanRespTime, 1-exp))
			erwpData.append([int(renTime)/60.0, erwp, exp])
	
	erwpData = np.array(erwpData)
	plotData = []
	for exp in w:
		mask = (erwpData[:,2] == exp)
		subMatrix = erwpData[mask]
		subMatrix[:,0] = 1.0 / subMatrix[:,0]
		subMatrix = subMatrix[subMatrix[:,0].argsort()]
		xs = subMatrix[:,0]
		ys = subMatrix[:,1]
		coeff = np.polyfit(xs.flatten(), ys.flatten(), 5)
		p = np.poly1d(coeff)
		newys = p(xs).tolist()
		plotData.append((xs.tolist(), p(xs), "w: {}".format(exp)))
		#plotData.append((xs.tolist(), ys, "w: {}".format(exp)))
	
	titles = {
		"title": "Full Offloading Model",
		"x": "Reneging Rate r",
		"y": "ERWP"
	}
	filePath = os.path.join(outputDir, "FullOffloading_ERWP_RenegingRate.png")
	plotGraph(getTupleValues(0, plotData), getTupleValues(1, plotData), titles, getTupleValues(2, plotData), savePath=filePath)


def computeBatchMetrics(data, metric="MRT", arg=None):
	assert metric in ["MRT", "MEC", "ERWP"]
	if metric == "ERWP":
		assert arg != None
	
	metricInfo = (metric + " with w: {}".format(arg)) if metric == "ERWP" else metric
	print("Computing confidence intervals for {}...".format(metricInfo))
	dataToWrite = {}
	
	for deadline, seedsData in data.items():
		values = list(seedsData.values())
		generalMean = np.mean(values)
		variance = np.var(values, ddof=1)
		fractionCoeff = np.sqrt(variance/len(values))
		minVal, maxVal = stats.t.interval(0.90, len(values)-1, loc=generalMean, scale=fractionCoeff)
			
		config = "total"
		dataToWrite.setdefault(config, {})[int(deadline)] = {
			"mean": generalMean,
			"variance": variance,
			"minVal": minVal,
			"maxVal": maxVal
		}
			
		#print("mean: {:.3f}, variance: {:.3f}, min: {:.3f}, max: {:.3f}".format(generalMean, variance, minVal, maxVal))

	return dataToWrite


def prepareERWPDataForBatchMetrics(responseData, energyData, w):
	erwpValues = {}
	for exp in w:
		erwpValues[exp] = {}
		for deadline, seedsVals in responseData.items():
			erwpValues[exp][deadline] = {}
			for seed in seedsVals.keys():
				if seed not in responseData[deadline] or seed not in energyData[deadline]:
					continue
				mrt = responseData[deadline][seed]
				mec = energyData[deadline][seed]
				erwp = np.multiply(np.power(mec, exp), np.power(mrt, 1-exp))
				erwpValues[exp][deadline][seed] = erwp
	
	return erwpValues


def writeBatchMetrics(intervalsData, metric, outputDir, args=None):
	for config, values in intervalsData.items():
		filePath = os.path.join(outputDir, "ConfidenceIntervals_{}{}_{}.csv".format(metric, ("_{}".format(args)) if args != None else "", config))
		
		with open(filePath, "w+", encoding="utf-8") as csv:
			print("Deadline [s], Deadline [min], Reneging Rate r, Mean, Variance, Left Value, Right Value", file=csv)
			
			for deadline, interval in values.items():
				deadlineMin = deadline // 60
				rRate = 1.0 / deadlineMin
				mean = interval["mean"]
				var = interval["variance"]
				left = interval["minVal"]
				right = interval["maxVal"]
				print("{}, {}, {:.4f}, {:.4f}, {:.4f}, {:.4f}, {:.4f}".format(deadline, deadlineMin, rRate, mean, var, left, right), file=csv)	
	

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	parser.add_argument("--inputDir", type=str, required=True)
	parser.add_argument("--outputDir", type=str, required=True)
	args = parser.parse_args()
	plotsPath = os.path.join(args.outputDir, "plots")
	csvPath = os.path.join(args.outputDir, "csv")
	os.makedirs(plotsPath, exist_ok=True)
	os.makedirs(csvPath, exist_ok=True)
	
	data, keys = loadData(args.inputDir, "BatchExecution")
	setupPlots()
	
	responseData, respDataRaw = computeMeanResponseTime(data, keys, plotsPath)
	energyData, enDataRaw = computeMeanEnergyConsumption(data, keys, plotsPath)
	computeERWP(responseData, energyData, keys, plotsPath, w=[0.1, 0.5, 0.9])
			
	respIntervals = computeBatchMetrics(respDataRaw)
	writeBatchMetrics(respIntervals, "MRT", csvPath)
	
	enIntervals = computeBatchMetrics(enDataRaw, metric="MEC")
	writeBatchMetrics(enIntervals, "MEC", csvPath)
	
	erwpRaw = prepareERWPDataForBatchMetrics(respDataRaw, enDataRaw, [0.1, 0.5, 0.9])
	for exp, rawData in erwpRaw.items():
		erwpIntervals = computeBatchMetrics(rawData, metric="ERWP", arg=exp)
		writeBatchMetrics(erwpIntervals, "ERWP", csvPath, "w_{}".format(exp))
	


