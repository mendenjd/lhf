#pragma once
#include "simplexBase.hpp"
#include <set>

// Header file for simplexTree class - see simplexTree.cpp for descriptions

class simplexTree : public simplexBase {
  private:
	struct treeNode{
		unsigned index;
		treeNode* child = nullptr;
		treeNode* sibling = nullptr;
		treeNode* parent = nullptr;
		double weight = 0;
	};
	
	bool isSorted = false;
	int maxDim;
	bool isLeaf;
	unsigned indexCounter;
	treeNode* head;
	int nodeCount;
	std::vector<treeNode*> dimensions;	
	std::vector<std::vector<std::pair<std::set<unsigned>, double>>> weightEdgeGraph;

	void printTree(treeNode*);
	bool haveChild(simplexTree const*);
	void insertInductive();
	void recurseInsert(treeNode*, unsigned, int, double, std::set<unsigned>);
  
  public:
	simplexTree(std::vector<std::vector<double>>);
	simplexTree(double, std::vector<std::vector<double>>, int);
	
	//virtual interface functions
	double getSize();
	void insert(std::vector<double>&);
	void find(std::vector<double>);
	int simplexCount();
	int vertexCount();
	std::vector<std::vector<unsigned>> getDimEdges(int,double);
	std::vector<std::vector<std::pair<std::set<unsigned>,double>>> getAllEdges(double);
	bool deletion(simplexTree*&, std::string);
	void expandDimensions(int){return;};
	void reduceComplex();
	
};

