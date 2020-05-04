#include <string>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <iostream>
#include "simplexTree.hpp"

simplexTree::simplexTree(double _maxEpsilon, std::vector<std::vector<double>> _distMatrix, int _maxDim){
	indexCounter = 0;
	std::cout << "simplexTree set indexCounter" << std::endl;
	distMatrix = _distMatrix;
	maxDimension = _maxDim;
	maxEpsilon = _maxEpsilon;
	simplexType = "simplexTree";
	return;
}

//**													**//
//** 				Private Functions 					**//
//**													**//
void simplexTree::recurseInsert(simplexNode* node, unsigned curIndex, int depth, double maxE, std::set<unsigned> simp){
	//Incremental insertion
	//Recurse to each child (which we'll use the parent pointer for...)
	simplexNode* temp;
	
	isSorted = false;
	double curE = 0;

	//std::cout << runningVectorIndices.size() << "\t" << runningVectorCount << "\t" << indexCounter << "\t" << distMatrix.size() << "\t" << node->index << std::endl;
	if(runningVectorIndices.size() < runningVectorCount+1){
		int offset = runningVectorCount+1 - runningVectorIndices.size();
		if(((int)node->index - offset) > distMatrix.size() || (indexCounter-offset) > distMatrix[(int)node->index - offset].size()){
			std::cout << "DistMatrix access error:" << std::endl;
			std::cout << "DistMatrix size: " << distMatrix.size() << "\tAccess Index: " << ((int)node->index - offset) << std::endl;
			std::cout << "Node Index: " << node->index << "\tOffset: " << offset << std::endl;
			std::cout << "nodeCount: " << nodeCount << "\tindexCount: " << indexCounter << std::endl;
		}
		else
			curE = distMatrix[(int)node->index - offset][indexCounter - offset];

	}else{
		curE = distMatrix[node->index][indexCounter];
	}

	curE = curE > maxE ? curE : maxE;

	//Check if the node needs inserted at this level
	if(curE < maxEpsilon){
		simplexNode* insNode = new simplexNode();
		insNode->index = curIndex;
		insNode->simplex = simp;
		insNode->simplex.insert(curIndex);
		insNode->sibling = nullptr;
		insNode->child = nullptr;
		nodeCount++;
		simp.insert(node->index);

		//Get the largest weight of this simplex
		insNode->weight = curE > node->weight ? curE : node->weight;
		
		//if depth (i.e. 1 for first iteration) is LT weightGraphSize (starts at 1)
		if(simplexList.size() < simp.size()){
			std::vector<simplexNode*> tempWEG;
			tempWEG.push_back(insNode);
			simplexList.push_back(tempWEG);
		} else {
			simplexList[simp.size() - 1].push_back(insNode);
		}

		//Check if the node has children already...
		if(node->child == nullptr){
			node->child = insNode;
			insNode->parent = node;

		} else {
			temp = node->child;
			while(temp->sibling != nullptr)
				temp = temp->sibling;
			
			temp->sibling = insNode;
			insNode->parent = temp->parent;
			temp = node->child;
			//Have to check the children now...
			if(simp.size() <= maxDimension){
				do {
					if(temp != insNode)
						recurseInsert(temp, curIndex, depth + 1, maxE, simp);
				} while(temp->sibling != nullptr && (temp = temp->sibling) != nullptr);
			}
		}
	}

	return;
}


void simplexTree::printTree(simplexNode* head){
	std::cout << "_____________________________________" << std::endl;
	if(head == nullptr){
		std::cout << "Empty tree... " << std::endl;
		return;
	}

	std::cout << "HEAD: " << head->index << "\t" << head << "\t" << head->child << "\t" << head->sibling << std::endl;

	simplexNode* current;
	for(int i = 0; i < simplexList.size() ; i++){
		std::cout << std::endl << "Dim: " << i << std::endl << std::endl;
		current = simplexList[i][0];

		do{
			std::cout << current->index << "\t" << current << "\t" << current->sibling << "\t" << current->child << "\t" << current->parent << std::endl;
		} while(current->sibling != nullptr && (current = current->sibling) != nullptr);

		std::cout << std::endl;
	}

	std::cout << "_____________________________________" << std::endl;
	return;
}



//**													**//
//** 				Public Functions 					**//
//**													**//


// Insert a node into the tree using the distance matrix and a vector index to track changes
bool simplexTree::insertIterative(std::vector<double> &currentVector, std::vector<std::vector<double>> &window){
	if(window.size() == 0){
		return true;
	}

	if(streamEval(currentVector, window)) {   // Point is deemed 'significant'
		std::vector<double> distMatrixRow = ut.nearestNeighbors(currentVector, window);

		//auto tempIndex = runningVectorIndices[0]->sibling;
		deleteIterative(runningVectorIndices[0]);
		//head = tempIndex;
		
		runningVectorIndices.erase(runningVectorIndices.begin());

		distMatrix.push_back(distMatrixRow);
		insert(distMatrixRow);

		removedSimplices++;

		return true;
	}

	return false;
}

// Insert a node into the tree using the distance matrix and a vector index to track changes
bool simplexTree::insertIterative(std::vector<double> &currentVector, std::vector<std::vector<double>> &window, int &keyToBeDeleted, int &indexToBeDeleted, std::vector<double> &distsFromCurrVec){
	if(window.size() == 0){
		return true;
	}

	if(streamEval(currentVector, window)) {   // Point is deemed 'significant'

		// deleteIterative(keyToBeDeleted);
		runningVectorIndices.erase(runningVectorIndices.begin() + indexToBeDeleted);

		insert(distsFromCurrVec);

		removedSimplices++;

		return true;
	}

	return false;
}


// Delete a node from the tree and from the distance matrix using a vector index
void simplexTree::deleteIterative(int vectorIndex){
	//Find what row/column of our distance matrix pertain to the vector index
	std::vector<int>::iterator it;
	if((it = std::find(runningVectorIndices.begin(), runningVectorIndices.end(), vectorIndex)) != runningVectorIndices.end()){

		int index = it - runningVectorIndices.begin();

		//Delete the row and column from the distance matrix based on vector index
		if(index > 0){
			
			distMatrix.erase(distMatrix.begin() + index);
		
			for(auto z : distMatrix){
				if(z.size() >= index)
					z.erase(z.begin() + index);
			}
		}

		auto curNodeCount = nodeCount;

		//Delete all entries in the simplex tree with the index...
		deleteIndexRecurse(vectorIndex, head);
		
		deleteWeightEdgeGraph(vectorIndex);
		
		//std::cout << "Node Count reduced from " << curNodeCount << " to " << nodeCount << std::endl;
		

	} else {
		ut.writeDebug("simplexTree","Failed to find vector by index");
	}
	return;
}


void simplexTree::deleteIndexRecurse(int vectorIndex) {

    deleteIndexRecurse(vectorIndex, head);
    return;
}


void simplexTree::deleteIndexRecurse(int vectorIndex, simplexNode* curNode){
	//std::cout << "deleteIndexRecurse: " << vectorIndex << std::endl;
	if(curNode == nullptr){
		std::cout << "Empty tree" << std::endl;
		return;
	}
	
	//Handle siblings - either they need to be removed (and 'hopped') or recursed
	if(curNode->sibling != nullptr && curNode->sibling->index == vectorIndex){
		simplexNode* tempNode = curNode->sibling;
		curNode->sibling = curNode->sibling->sibling;
		deleteIndexRecurse(vectorIndex, curNode->sibling);

	} else if(curNode->sibling != nullptr){
		deleteIndexRecurse(vectorIndex, curNode->sibling);
	}

	if(curNode->index == vectorIndex){

		if(curNode == head){
			head = curNode->sibling;
			//simplexList[0].insert(simplexList.begin(), curNode->sibling); 
		}
		
		for(auto d : simplexList){
			if(d[0] == curNode)
				d.insert(d.begin(), curNode->sibling);
		}
			
		deletion(curNode);
	} else if (curNode->child != nullptr && curNode->child->index == vectorIndex){
		simplexNode* tempNode = curNode->child;
		curNode->child = curNode->child->sibling;
		
		for(auto d : simplexList){
			if(d[0] == tempNode)
				d.insert(d.begin(), tempNode->sibling);
		}
		
		deletion(tempNode);

	} else if(curNode->child != nullptr){
		deleteIndexRecurse(vectorIndex, curNode->child);
	}

	return;
}



// Insert a node into the tree
//
void simplexTree::insert(std::vector<double>&) {
	
	if(distMatrix.size() == 0){
		ut.writeDebug("simplexTree","Distance matrix is empty, skipping insertion");
		return;
	}

	//Create our new node to insert
	simplexNode* curNode = new simplexNode;
	curNode->index = indexCounter;
	std::set<unsigned> tempSet = {curNode->index};
	runningVectorIndices.push_back(indexCounter);

	//Check if this is the first node (i.e. head)
	//	If so, initialize the head node
	if(head == nullptr){
		head = curNode;
		indexCounter++;
		runningVectorCount++;
		nodeCount++;

		std::vector<simplexNode*> tempWEG;
		tempWEG.push_back(curNode);
		simplexList.push_back(tempWEG);

		return;
	}

	// This needs to be a recursive span -> (or not!)
	//		if the node has a child, recurse
	//		if the node has a sibling, recurse
	//
	//d0 -->          ({!})
	//			       /
	//			      /
	//d1 -->	   | 0 | 1 | ... |
	//		        /
	//             /
	//d2 -->    | 1 | 2 | 3 |
	//           /         \
	//	        /           \
	//d3 --> | 2 | 3 |     | 4 | 5 |
	//

	simplexNode* temp = simplexList[0][0];

	//Now let's do it the new way - incremental VR;
	//	Start at the first dimension (d0);
	//		if e (d0, dn) < eps
	//			recurse down tree
	//				recurse
	//			insert to current
	//	iterate to d0->sibling

	do{
		recurseInsert(temp, indexCounter, 0, 0, tempSet);
	}while(temp->sibling != nullptr && (temp = temp->sibling) != nullptr);

	//Insert into the right of the tree
	temp = simplexList[0][0];
	simplexNode* ins = new simplexNode();
	while(temp->sibling != nullptr && (temp = temp->sibling) != nullptr);
	ins->index = indexCounter;
	ins->sibling = nullptr;
	ins->parent = nullptr;
	temp->sibling = ins;
	simplexList[0].push_back(ins);

	nodeCount++;
	indexCounter++;
	runningVectorCount++;
	return;
}

void simplexTree::deleteWeightEdgeGraph(int index){
	
	for(unsigned dim = 0; dim < simplexList.size(); dim++){
		std::vector<unsigned> d_indices;
		
		for(int vInd = simplexList[dim].size()-1; vInd >= 0; vInd--){
			//std::cout << "vInd: " << vInd << std::endl;
			
			if(simplexList[dim][vInd]->simplex.find(index) != simplexList[dim][vInd]->simplex.end())
				d_indices.push_back(vInd);
		}
		
		int rem = 0;
		for(auto del : d_indices){
			rem++;
			simplexList[dim].erase(simplexList[dim].begin() + del);
		}
	}
	return;
}


int simplexTree::vertexCount(){
	//Return the number of vertices currently represented in the tree
	if(runningVectorIndices.size() < runningVectorCount+1)
		return runningVectorIndices.size();
	return indexCounter;
}

int simplexTree::simplexCount(){
	//Return the number of simplices currently in the tree
	return nodeCount;
}

double simplexTree::getSize(){
	//Size of node: [int + byte (*) + byte (*)] = 18 Bytes
	return nodeCount * sizeof(simplexNode);
}

void simplexTree::reduceComplex(){
	if(simplexList.size() == 0){
		ut.writeDebug("simplexTree","Complex is empty, skipping reduction");
		return;
	}

	//Start with the largest dimension
	ut.writeDebug("simplexTree","Reducing complex, starting simplex count: " + std::to_string(simplexCount()));
	simplexBase::simplexNode* cur;

	if(simplexList.size() > 0){
		for(auto i = simplexList.size()-1; i > 1; i--){

			std::vector<std::set<unsigned>> removals;
			std::vector<std::set<unsigned>> checked;

			while(checked.size() != simplexList[i].size()){
				//cur = simplexList[i];
				do {
					if(std::find(checked.begin(),checked.end(),cur->simplex) == checked.end()){
						auto ret = recurseReduce(cur, removals, checked);
						removals = ret.first;
						checked = ret.second;
					}
				} while (cur->sibling != nullptr && (cur = cur->sibling) != nullptr);
			}

			//Remove the removals
			for(auto rem : removals){
				deletion(rem);
			}

		}
	}
	ut.writeDebug("simplexTree","Finished reducing complex, reduced simplex count: " + std::to_string(simplexCount()));

	return;
}

std::pair<std::vector<std::set<unsigned>>, std::vector<std::set<unsigned>>> simplexTree::recurseReduce(simplexNode* simplex, std::vector<std::set<unsigned>> removals, std::vector<std::set<unsigned>> checked){
	checked.push_back(simplex->simplex);
	auto subsets = ut.getSubsets(simplex->simplex);
	std::set<unsigned> maxFace;

	bool canRemove = true;

	//Look at each face
	for(auto face : subsets){

		//Check if the face is shared; if so, recurse
		for(auto simp : simplexList[simplex->simplex.size() - 1]){

			if(simp != simplex && std::find(checked.begin(), checked.end(), simp->simplex) == checked.end()){
				auto sDiff = ut.symmetricDiff(simp->simplex, face,true);

				//This point intersects;
				if (sDiff.size() == 1){
					auto ret = recurseReduce(simp, removals, checked);
					removals = ret.first;
					checked = ret.second;

					//Check if the simplex was not removed
					if(std::find(removals.begin(), removals.end(), simp->simplex) == removals.end()){
						canRemove = false;
						break;
					}
				}

			}
		}

		//Check if the face is the max face
		double wt = -1;
		//if((wt = findWeight(face)) == simplex->weight){
		//	maxFace = face;
		//}

	}

	if(canRemove){
		removals.push_back(simplex->simplex);
		removals.push_back(maxFace);
	}

	return std::make_pair(removals, checked);

}

bool simplexTree::find(std::set<unsigned>){

	ut.writeLog("simplexTree","find(std::set<unsigned>) not implemented!");
	return 0;
}

// A recursive function to delete a simplex (and sub-branches) from the tree.
bool simplexTree::deletion(std::set<unsigned> removalEntry) {
	//Remove the entry in the simplex tree
	bool found = true;
	simplexNode* curNode = simplexList[0][0];
	int d = removalEntry.size() - 1;


	/*for(int i = 0; i < dimensions[d].size(); i++){
		if(removalEntry == dimensions[d][i]->simplexSet){
			curNode = dimensions[d][i];
			dimensions[d]->erase(dimensions[d].begin() + i);
			return deletion(curNode);
		}
	}*/


	ut.writeLog("simplexTree","deletion(std::set<unsigned>) not implemented!");
	return false;

}


// A recursive function to delete a simplex (and sub-branches) from the tree.
bool simplexTree::deletion(simplexNode* removalEntry) {
	simplexNode* curNode = removalEntry;

	//Iterate to the bottom of branch in the current node
	while(curNode->child != nullptr){
		curNode->child->parent = curNode;
		curNode=curNode->child;
	}
	
	if(curNode == removalEntry && curNode->sibling != nullptr){
		deletion(curNode->sibling);
	}

	//If we did go down, remove on the way back up
	while(curNode != removalEntry){
		if(curNode->sibling != nullptr){
			deletion(curNode->sibling);
		}
		curNode = curNode->parent;

		nodeCount--;
		delete curNode->child;
		curNode->child = nullptr;
	}

	//curNode = curNode->parent;
	nodeCount--;
	delete curNode;
	return false;
}

void simplexTree::clear(){

	//Clear the simplexTree structure
	deletion(head);
	head = nullptr;

	//Clear the weighed edge graph
	for(auto z : simplexList){
		z.clear();
	}
	simplexList.clear();


	//runningVectorCount = 0;
	runningVectorIndices.clear();
	//indexCounter = 0;

}

