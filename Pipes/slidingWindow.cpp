/*
 * streamVR hpp + cpp extend the basePipe class for
 *
 */

#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <iterator>
#include <vector>
#include <chrono>
#include <functional>
#include "slidingWindow.hpp"
#include "utils.hpp"
#include "readInput.hpp"


// basePipe constructor
slidingWindow::slidingWindow(){
	pipeType = "SlidingWindow";
	return;
}

// runPipe -> Run the configured functions of this pipeline segment
pipePacket slidingWindow::runPipe(pipePacket inData){
	utils ut;
	readInput rp;

	int windowMaxSize = 50;

	// For this pipe, we construct a sub-pipeline:
	//		1. Read data vector by vector, push into slidingWindow evaluation
	//		2. IF the point is to be inserted, push into the simplexTree (LRU ordered)
	//		3. IF 100 points have passed, generate persistence intervals
	//
	// Loop this subpipeline until there's no more data

	std::vector<int> windowKeys;
	std::vector<std::vector<double>> windowValues;

	std::vector<int> partitionLabels;
	std::vector<int> nnIndices;  // A container to store the index of each point's nearest neighbor within the window.
	std::vector<double> nnDists;  // A container to store the nearest neighbor distance of each point within the window.
	std::unordered_map<int, double> avgNNDistPartitions;

	int key{ 0 };
	float f1{ 4 };
	float f2{ 0.25 };

	// A partition is considered outdated if it did not receive any new point for more than the last 25 insertions.
	int timeToBeOutdated{ 25 };

	std::unordered_map<int, int> numPointsPartn;  // A dictionary to store the number of points in each partition.
	std::unordered_map<int, int> maxKeys;  // A dictionary to store the maxKey of each partition.

	std::vector<double> currentVector;
	if(rp.streamInit(inputFile)){
		int pointCounter = 1;
		int label{ 0 };

		while(rp.streamRead(currentVector)){
			// Initialize the sliding window. During the initialization, let's assume all points from the stream
			// belong to Partition 0.
			if(windowValues.size() < windowMaxSize){
				windowValues.push_back(currentVector);
				windowKeys.push_back(key);
				partitionLabels.push_back(label);
				key++;

				//If we've reached window max size, generate the initial complex
				if(windowValues.size() == windowMaxSize){
					std::cout << "Initializing complex" << std::endl;

					inData.originalData = windowValues;
					runComplexInitializer(inData);
					
					std::cout << "Returning from complex initializer" << std::endl;
				}

			} else {

				if(inData.complex->insertIterative(currentVector, windowValues)){

					windowValues.erase(windowValues.begin());

					//Insert the point
					windowValues.push_back(currentVector);
				}
			}

			//Check if we've gone through 100 points
			if(pointCounter % 100 == 0 && pointCounter > windowMaxSize){
				// Build and trigger remaining pipeline. It should only require the computation of persistence
				// intervals from the complex being maintained.
				std::cout << "pointCounter: " << pointCounter << "\tSimplex Count: " << inData.complex->simplexCount() << "\tVertex Count: " << inData.complex->vertexCount() << std::endl;
				std::cout << "\tWindowSize: " << windowValues.size() << std::endl;
				inData.originalData = windowValues;
				runSubPipeline(inData);

			}
			pointCounter++;

			currentVector.clear();

		}
		//Probably want to trigger the remaining pipeline one last time...
		if((pointCounter - 1) % 100 != 0){
			std::cout << "pointCounter: " << pointCounter << "\tSimplex Count: " << inData.complex->simplexCount() << "\tVertex Count: " << inData.complex->vertexCount() << std::endl;

			runSubPipeline(inData);
		}
		ut.writeLog("slidingWindow", "\tSuccessfully evaluated " + std::to_string(pointCounter) + " points");

		writeComplexStats(inData);
	}

	return inData;
}

void slidingWindow::writeComplexStats(pipePacket &inData){
	if(inData.complex->stats.size() > 30){
		std::ofstream file ("output/complexStats.csv");

		file << inData.complex->stats << std::endl;

		file.close();

	}
}

void slidingWindow::runSubPipeline(pipePacket wrData){
    if(wrData.originalData.size() == 0)
		return;

	pipePacket inData = wrData;
	outputData(inData);

	std::string pipeFuncts = "rips.persistence";
	auto lim = count(pipeFuncts.begin(), pipeFuncts.end(), '.') + 1;
	subConfigMap["fn"] = "_" + std::to_string(repCounter);
	repCounter++;

	//For each '.' separated pipeline function (count of '.' + 1 -> lim)
	for(unsigned i = 0; i < lim; i++){
		auto curFunct = pipeFuncts.substr(0,pipeFuncts.find('.'));
		pipeFuncts = pipeFuncts.substr(pipeFuncts.find('.') + 1);

		//Build the pipe component, configure and run
		auto *bp = new basePipe();
		auto *cp = bp->newPipe(curFunct, "simplexTree");

		//Check if the pipe was created and configure
		if(cp != 0 && cp->configPipe(subConfigMap)){
			//Run the pipe function (wrapper)
			inData = cp->runPipeWrapper(inData);
		} else {
			std::cout << "LHF : Failed to configure pipeline: " << curFunct << std::endl;
		}
	}


	return;
}

void slidingWindow::runComplexInitializer(pipePacket &inData){
	//Initialize the complex and build other structures for maintaining NN, etc.
	//
	//	We need to exit this function by covering the distMatrix and neighGraph 
	//		pipe functions
	
	
	
	//	1.	Create distance matrix (and compute other info)-------------
	
	//Store our distance matrix
	std::vector<std::vector<double>> distMatrix (inData.originalData.size(), std::vector<double>(inData.originalData.size(),0));
	
	//Iterate through each vector
	for(unsigned i = 0; i < inData.originalData.size(); i++){
		if(!inData.originalData[i].empty()){
		
			//Grab a second vector to compare to 
			std::vector<double> temp;
			for(unsigned j = i+1; j < inData.originalData.size(); j++){

					//Calculate vector distance 
					auto dist = ut.vectors_distance(inData.originalData[i],inData.originalData[j]);
					
					if(dist < epsilon)
						inData.weights.insert(dist);
					distMatrix[i][j] = dist;
			}
		}
	}
	
	inData.complex->setDistanceMatrix(distMatrix);
	
	inData.weights.insert(0.0);
	inData.weights.insert(epsilon);
	//std::sort(inData.weights.begin(), inData.weights.end(), std::greater<>());
	
	//------------------------------------------------------------------
	
	
	
	// 2. Insert into complex (build neighborhood graph) ---------------
	
	//Iterate through each vector, inserting into simplex storage
	for(unsigned i = 0; i < inData.originalData.size(); i++){
		if(!inData.originalData[i].empty()){
			//insert data into the complex (SimplexArrayList, SimplexTree)
			inData.complex->insert(inData.originalData[i]);	
		}
	}
	
	//------------------------------------------------------------------
	
	
	return;
}



// configPipe -> configure the function settings of this pipeline segment
bool slidingWindow::configPipe(std::map<std::string, std::string> configMap){
	std::string strDebug;
	subConfigMap = configMap;

	auto pipe = configMap.find("debug");
	if(pipe != configMap.end()){
		debug = std::atoi(configMap["debug"].c_str());
		strDebug = configMap["debug"];
	}
	pipe = configMap.find("outputFile");
	if(pipe != configMap.end())
		outputFile = configMap["outputFile"].c_str();

	ut = utils(strDebug, outputFile);

	pipe = configMap.find("inputFile");
	if(pipe != configMap.end())
		inputFile = configMap["inputFile"].c_str();

	pipe = configMap.find("epsilon");
	if(pipe != configMap.end())
		epsilon = std::atof(configMap["epsilon"].c_str());
	else return false;

	pipe = configMap.find("dimensions");
	if(pipe != configMap.end()){
		dim = std::atoi(configMap["dimensions"].c_str());
	}
	else return false;

	configured = true;
	ut.writeDebug("slidingWindow","Configured with parameters { input: " + configMap["inputFile"] + ", dim: " + configMap["dimensions"] + ", eps: " + configMap["epsilon"] + ", debug: " + strDebug + ", outputFile: " + outputFile + " }");

	return true;
}


// outputData -> used for tracking each stage of the pipeline's data output without runtime
void slidingWindow::outputData(pipePacket inData){
	std::ofstream file ("output/" + pipeType + "_" + std::to_string(repCounter) + "_output.csv");

	for(auto a : inData.originalData){
		for(auto d : a){
			file << d << ",";
		}
		file << "\n";
	}
	file << std::endl;

	file.close();
	return;
}
