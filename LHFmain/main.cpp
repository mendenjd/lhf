#include "mpi.h"
#include "omp.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <thread>
#include "LHF.hpp"
#include <string> 

int main(int argc, char* argv[]){
	double start = omp_get_wtime();
	//	Steps to compute PH with preprocessing:
	//
	//		1.  Preprocess the input data
	//			a. Store the original dataset, reduced dataset, cluster indices
	//
	//		2.	Build a 2-D neighborhood graph
	//			a. Weight each edge less than epsilon
	//			b. Order edges (min to max)
	//
	//		3.  Rips filtration
	//			a. Build higher-level simplices (d > 2)
	//				- Retain the largest of edges as weight (when simplex forms)
	//			b. Build list of **relevant epsilon values
	//
	//		4.	Betti calculation
	//			a. For each relevant epsilon value
	//				i.	Create Boundary Matrix
	//					- if pn[i,j] < n, set to 1
	//					- else set to 0
	//				ii.	Compute RREF
	//				iii.Store constituent boundary points
	//
	//		5.	Upscaling
	//			a. Upscale around boundary points
	//			b. For each upscaled boundary, repeat steps 2-4
	//
	//
	//	**relevant refers to edge weights, i.e. where a simplex of any
	//			dimension is created (or merged into another simplex)
	
	
	//Define external classes used for reading input, parsing arguments, writing output
	auto lhflib = LHF();
	auto rs = readInput();
	
	//Parse the command-line arguments
	auto args = argParser::parse(argc, argv);
	
	//Determine what pipe we will be running
	argParser::setPipeline(args);
	
	//Create a pipePacket (datatype) to store the complex and pass between engines
	auto wD = pipePacket(args, args["complexType"]);	//wD (workingData)
	
	if(args["pipeline"] != "slidingwindow" && args["pipeline"] != "naivewindow" && args["mode"] != "mpi"){
		//Read data from inputFile CSV
		wD.inputData = rs.readCSV(args["inputFile"]);
		wD.workData = wD.inputData;
	}

	//If data was found in the inputFile
	if(wD.inputData.size() > 0 || args["pipeline"] == "slidingwindow" || args["pipeline"] == "naivewindow" || args["mode"] == "mpi"){

		if(args["mode"] == "mpi"){
			
			MPI_Init(&argc,&argv);
			MPI_Comm_size(MPI_COMM_WORLD,&lhflib.nprocs);
			MPI_Comm_rank(MPI_COMM_WORLD,&lhflib.id);
			
			wD.bettiTable = lhflib.processUpscaleWrapper(args, wD);
			sort(wD.bettiTable.begin(), wD.bettiTable.end(), sortBettis());
			MPI_Finalize();

		} else if(args["mode"] == "reduced" || args["mode"] == "iterUpscale" || args["mode"] == "iter"){	
			wD.bettiTable = lhflib.processIterUpscale(args,wD);
			sort(wD.bettiTable.begin(), wD.bettiTable.end(), sortBettis());
			
		} else {
			lhflib.processDataWrapper(args, wD);
			lhflib.runPipeline(args, wD);
		}

	} else {
		argParser::printUsage();
	}
		
	if((args["debug"] == "1" || args["debug"] == "true") && wD.bettiTable.size() > 0 ){
		std::cout << std::endl << "_______Merged BETTIS_______" << std::endl;

		for(auto a : wD.bettiTable){
			std::cout << a.bettiDim << ",\t" << a.birth << ",\t" << a.death << ",\t";
			utils::print1DVector(a.boundaryPoints);
		}
	}
	
	delete wD.complex;

	double end = omp_get_wtime();
	std::cout << "Total LHF execution time (s): " << end-start << std::endl;

	return 0;
}
