/*
 * basePipe hpp + cpp protoype and define a base class for building
 * pipeline functions to execute
 *
 */
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <random>
#include <chrono>
#include <string>
#include <numeric>
#include <iostream>
#include <functional> 
#include <vector>
#include "kMeansPlusPlus.hpp"
#include "utils.hpp"

// basePipe constructor
kMeansPlusPlus::kMeansPlusPlus(){
	procName = "k-means++";
    return;
}
//taking in preprocessor type


// runPipe -> Run the configured functions of this pipeline segment
void kMeansPlusPlus::runPreprocessor(pipePacket& inData){
	if(!configured){
		ut.writeLog(procName,"Preprocessor not configured");
		return;
	}
	
    //Arguments - num_clusters, num_iterations
    std::vector<int> labels; //Storing labels for mapping data to centroids
    unsigned dim = inData.workData[0].size();

    //Initialize centroids (Plus plus mechanism with kmeans - Hartigan, Wong)
    
    //initialize first random centroid from data
    
    //mersenne twister random algorithm - used to have reproducible results from seed
    //  This seed should be recorded to reproduce after a run
    //  There may be multiple seeds in a run depending on how many times k-means is used
    //static std::random_device seed;
    if(seed < 0){
		seed = time(NULL);
    }
    static std::mt19937 gen(seed); 
    
    std::uniform_int_distribution<size_t> distribution(0, inData.workData.size()-1);
    int index = distribution(gen); //Randomly choose the first centroid

    std::vector<std::vector<double>> centroids(num_clusters, std::vector<double>(dim, 0)); 
    std::vector<double> dist(inData.workData.size(), std::numeric_limits<double>::max()); //Distance to nearest centroid

    centroids[0] = inData.workData[index]; //Adding first mean to centroids

	//Determining initial centroids by probability / distance from previous centroid
	// 	Do we need to compare the picked centroid to all centroids previously picked 
	//		-> in order to maximize the distance/difference between centroids
    for(unsigned k = 1; k<num_clusters; k++){ //adding means 2 -> k to centroids based on distance 
     	double maxDist = 0; //Choose the point that is farthest from its closest centroid
     	int clusterIndex = 0;

        for(unsigned j=0; j<inData.workData.size(); j++) {

			double curDist = ut.vectors_distance(inData.workData[j], centroids[k-1]);	
			if(curDist < dist[j]) dist[j] = curDist;

			if(dist[j] > maxDist){
				maxDist = dist[j];
				clusterIndex = j;
			}
     	}

        centroids[k] = inData.workData[clusterIndex];
    }

    //Now, we need to iterate over the centroids and classify each point
    //	Compute the new centroid as the geometric mean of the classified points
    //		IF the centroid has 0 points classified to it, we need to repick/reapproach (find out how)
    //	Replace old centroids with new centroids, rinse, repeat until convergence
    //		Convergence on # iterations, or a minimum movement of centroids (< .01)
    // OUTPUT: Final centroids, labeled data, sum of vectors in a label, count of points in label
    
    //Need to store: 
    //	Counts, the number of clusters in each classification
    //	summedClusters, the total cluster distance (for r_max and r_avg)
    //	summedCentroidVectors, for the geometric sum of the centroid
    //	lastCentroids, to track the change in cluster WCSSE
    std::vector<unsigned> lastLabels;
	std::vector<std::vector<double>> summedCentroidVectors(num_clusters, std::vector<double>(dim, 0));

    
    //Iterate until we reach max iderations or no change in the classification
	for (int z = 0; z < num_iterations; z++){		
		std::vector<unsigned> counts(num_clusters, 0);
		std::vector<unsigned> curLabels(inData.workData.size());
		
		//For each point, classify it to a centroid
		for (unsigned j = 0; j < inData.workData.size(); j++){
			double minDist = std::numeric_limits<double>::max();
			unsigned clusterIndex = 0;
			
			//Check each centroid for the minimum distance
			for (unsigned c = 0; c < centroids.size(); c++){
				auto curDist = ut.vectors_distance(inData.workData[j], centroids[c]);
				
				if(curDist < minDist){
					clusterIndex = c;
					minDist = curDist;
				}
			}
			
			if(z == 0){
				for(int d = 0; d < dim; d++)
					summedCentroidVectors[clusterIndex][d] += inData.workData[j][d];
			} else if(lastLabels[j] != clusterIndex){
				for(int d = 0; d < dim; d++){
					summedCentroidVectors[lastLabels[j]][d] -= inData.workData[j][d];
					summedCentroidVectors[clusterIndex][d] += inData.workData[j][d];
				}
			}
			curLabels[j] = clusterIndex;
			counts[clusterIndex]++;			
		}		
		
		//Otherwise, 
		//		Shift the centroid geometric centers to new centroids
		for(int i = 0; i < summedCentroidVectors.size(); i++){
			if(counts[i] == 0){
				centroids[i] = inData.workData[distribution(gen)];
			} else{
				for(int d = 0; d < summedCentroidVectors[0].size(); d++){
					centroids[i][d] = summedCentroidVectors[i][d] / counts[i];
				}
			}
		}
		
		//Check for convergence
		if(curLabels == lastLabels) break;
		lastLabels = curLabels;
		
	}
    
	//Assign to the pipepacket
	inData.workData = centroids;
	inData.centroidLabels = lastLabels;
	return;
}

// configPipe -> configure the function settings of this pipeline segment
bool kMeansPlusPlus::configPreprocessor(std::map<std::string, std::string>& configMap){
	std::string strDebug;
	
	auto pipe = configMap.find("debug");
	if(pipe != configMap.end()){
		debug = (std::atoi(configMap["debug"].c_str()) > 0 ? true : false);
		strDebug = configMap["debug"];
	}
	pipe = configMap.find("outputFile");
	if(pipe != configMap.end())
		outputFile = configMap["outputFile"].c_str();
	
	ut = utils(strDebug, outputFile);
    pipe = configMap.find("clusters");
    if(pipe !=configMap.end())
        num_clusters = std::atoi(configMap["clusters"].c_str());
    else return false;
    
    pipe = configMap.find("seed");
    if(pipe != configMap.end())
		seed = std::atoi(configMap["seed"].c_str());

    pipe = configMap.find("iterations");
	if(pipe != configMap.end())
		num_iterations = std::atoi(configMap["iterations"].c_str());
	else return false;
	
	configured = true;
	ut.writeDebug("StreamKMeans","Configured with parameters { clusters: " + configMap["clusters"] + ", iterations: " + configMap["iterations"] + ", debug: " + strDebug + ", outputFile: " + outputFile + " }");

	return true;
}
