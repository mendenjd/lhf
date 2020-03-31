#pragma once
#include <map>
#include <algorithm>
#include <set>
#include <iostream>
#include "utils.hpp"

// Header file for simplexBase class - see simplexTree.cpp for descriptions

class simplexBase {
  private:
  public:
	utils ut;
	double minDist = 0.0;
	double maxDist = 0.0;

	struct indTreeNode{
		unsigned index;
		unsigned sortedIndex;
		indTreeNode* child = nullptr;
		indTreeNode* sibling = nullptr;
		indTreeNode* parent = nullptr;
		std::set<unsigned> simplexSet;
		double weight = 0;
	};

	struct graphEntry{
		std::set<unsigned> simplexSet;
		double weight = 0;
		indTreeNode* entry = nullptr;

		graphEntry(){}
		graphEntry(std::set<unsigned> simp, double wt, indTreeNode* ent) {
			simplexSet = simp; weight = wt; entry = ent;
		}

		//Iteratively build subsets (faces) of the simplex set
		std::vector<std::set<unsigned>> getAllSubsets(std::set<unsigned> set){
			std::vector<std::set<unsigned>> subset;
			std::set<unsigned> empty;
			subset.push_back(empty);

			//For each set in the
			for(auto i = set.begin(); i!= set.end(); i++){
				std::vector<std::set<unsigned>> subsetTemp = subset;
				unsigned entry = *i;

				for (unsigned j = 0; j < subsetTemp.size(); j++){
					subsetTemp[j].insert(entry);
				}

				unsigned z = 0;
				for (auto j = subsetTemp.begin(); j != subsetTemp.end(); j++){
					subset.push_back(*j);

				}
			}

			std::vector<std::set<unsigned>> retSubset;

			for(std::set<unsigned> z : subset){
				if(z.size() == simplexSet.size() - 1)
					retSubset.push_back(z);
			}
			return retSubset;
		}


		std::set<unsigned> getFaces(simplexBase* simpTree){
			std::set<unsigned> indexes;

			std::vector<std::set<unsigned>> subsets = getAllSubsets(simplexSet);

			for(auto z : subsets){
				indexes.insert(simpTree->find(z));
			}

			return indexes;
		}
	};

	std::vector<unsigned> dimCounts = {6, 15, 20};
	std::string simplexType = "simplexBase";
	double maxEpsilon;
	int maxDimension;
	std::vector<std::vector<double>> distMatrix;
	std::vector<std::vector<std::pair<std::vector<unsigned>, double>>> weightedGraph;
	int runningVectorCount = 0;
	std::vector<int> runningVectorIndices;
	int removedSimplices = 0;
	std::string stats = "RVIndex,Mean,Stdev,k,kNN_Mean,kNN_Stdev,Result\n";

	simplexBase();
	simplexBase(std::map<std::string, std::string>);
	simplexBase(double, int);
	void setConfig(std::map<std::string, std::string>);
	void setDistanceMatrix(std::vector<std::vector<double>> _distMatrix);
	simplexBase* newSimplex(const std::string &simplexT, std::map<std::string, std::string> configMap);
	bool (*streamEval) (std::vector<double>&, std::vector<std::vector<double>>&);
    bool streamEvaluator(std::vector<double>&, std::vector<std::vector<double>>&);
	void setStreamEvaluator(bool (*f) (std::vector<double>&, std::vector<std::vector<double>>&));

	//virtual interface functions
	virtual double getSize();
	virtual bool insertIterative(std::vector<double>&, std::vector<std::vector<double>>&);
	virtual void deleteIterative(int, int);
	virtual void insert(std::vector<double>&);
	virtual bool find(std::vector<unsigned>);
	virtual bool find(std::set<unsigned>);
	virtual int simplexCount();
	virtual int vertexCount();
	virtual std::vector<std::vector<unsigned>> getDimEdges(int,double);
	virtual std::vector<std::vector<std::pair<std::set<unsigned>, double>>> getAllEdges(double);
	virtual std::vector<std::vector<graphEntry>> getIndexEdges(double);
	virtual void expandDimensions(int);
	virtual void reduceComplex();
	virtual std::vector<std::pair<double, std::vector<unsigned>>> getd0Pairs();
	virtual void clear();

	//Unused, possibly future
	virtual void outputSimplex();
};
