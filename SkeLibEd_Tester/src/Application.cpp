#include <iostream>
#include <limits.h>
#include "skelibed.hpp"
#include <chrono>
#include "normal_test.hpp"

//#define NITEMS 5000

std::vector<int> in(NITEMS);
std::vector<int> out(NITEMS);

// MAP TESTER:
// func 	- elemental function used in the map pattern
// input 	- input vector of items
// args		- arguments list
// ---------------------------------------------------
template<typename EL, typename IN, typename ...ARGs >
void mapTester(EL &func, IN &input, ARGs... args){

	// Get time of chrono call
	// -----------------------
	std::chrono::steady_clock::time_point chronoStart = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point chronoEnd = std::chrono::steady_clock::now();
	auto chronoCallTime = chronoEnd - chronoStart;

	// Initialise testing options
	// --------------------------
	int threadCount = 2;	// Starts at 2^1 continues to 2^10
	int blockCount = 2;		// Starts at 2^1 continues to 2^10
	int inputSize = 10;		// Starts at 10^1 continues to 10^10

	// Map skeleton
	auto map = Map(func, 50);

	// Execute Map skeleton
	auto mapStart = std::chrono::steady_clock::now();
	map(out, input, args...);
	auto mapEnd	  = std::chrono::steady_clock::now();
	std::cout << "Map timing (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
	std::cout << "Map timing (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
	std::cout << "Map timing (ns): " << std::chrono::duration_cast<std::chrono::nanoseconds> (mapEnd - mapStart - chronoCallTime).count() << std::endl;
	// for (i = 1; i <= 10; i++){	// increase task size
	// 	for (int b = 1; b <= 10 ; b++){
	// 		for (int t = 1; t <= 10; t++){
	//
	// 			std::cout<< "-------------------------" << '\n';
	// 			std::cout<< "inputSize:   " << inputSize << '\n';
	// 			std::cout<< "blockCount:  " << blockCount << '\n';
	// 			std::cout<< "threadCount: " << threadCount << '\n';
	//
	//
	//
	//
	// 			std::cout << "------------------------" << '\n';
	// 		}
	// 	}
	// }

}

// MAIN
int main()
{
	// input data
	for (size_t i = 0; i < NITEMS; i++) {
		in[i] = i;
	}
	// elemental argument
	int elemental_arg = 2;

	// call test map
	mapTester(elemental, in, elemental_arg);


	// Check output
	for (size_t i = 0; i < NITEMS; i++)
		std::cout << "Item [" << i << "] = " << out[i] << std::endl;

	//normal_main();
	// // Get time of chrono call
	// // -----------------------
	// std::chrono::steady_clock::time_point chronoStart = std::chrono::steady_clock::now();
	// std::chrono::steady_clock::time_point chronoEnd = std::chrono::steady_clock::now();
	// auto chronoCallTime = chronoEnd - chronoStart;
	// std::cout << "chrono call = " << std::chrono::duration_cast<std::chrono::microseconds>(chronoCallTime).count() << std::endl;
	// std::cout << "chrono call = " << std::chrono::duration_cast<std::chrono::nanoseconds> (chronoCallTime).count() << std::endl;
	//
	// // Calculate time of task
	// // ----------------------
	// std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	// for (int i = 1; i < 1000000; i++) {
	// 	int a;
	// }
	//
	// // Check time without precision
	// // ----------------------------
	// std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	// std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;
	// std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;
	//
	// // Remove chrono call time for better precision;
	// // ---------------------------------------------
	// end -= chronoCallTime;
	//
	// std::cout << "Time difference precision = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;
	// std::cout << "Time difference precision = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;


	std::cin.get();

}
