#include <iostream>
#include <limits.h>
#include "skelibed.hpp"
#include <chrono>
//#include "normal_test.hpp"


// ------------------------------------------------------
// MAP TESTER:
// func 	- elemental function used in the map pattern
// input 	- input vector of items
// args		- arguments list
// ------------------------------------------------------

template<typename EL, typename IN, typename OUT, typename ...ARGs >
void mapTester(EL &func, IN &input, OUT &output, ARGs... args){
	// Initialise testing options
	// --------------------------

	auto start = std::chrono::steady_clock::now();
	auto overallTime = start - start;
	/*
	std::cout << "Map timing (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
	std::cout << "Map timing (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
	std::cout << "Map timing (ns): " << std::chrono::duration_cast<std::chrono::nanoseconds> (mapEnd - mapStart - chronoCallTime).count() << std::endl;
	*/

	int blockCount = 2;
	for (int b = 1; b <= 5; b++){				// B = block size multiplicative
		int threadCount = 2;
	 	for (int pt = 1; pt <= 5; pt++){		// PT = ptreads size multiplicative
	
	 		std::cout<< "-------------------------" << '\n';
	 		std::cout<< "blockCount:  " << blockCount << '\n';
	 		std::cout<< "threadCount: " << threadCount << '\n';
			
			overallTime -= overallTime;
			int testCount = 50;
			int nulified = 0;
			for (int t = 1; t <= testCount; t++) {
				// Map skeleton
				auto map = Map(func, threadCount, blockCount);

				// Execute Map skeleton
				auto mapStart = std::chrono::steady_clock::now();
				map(output, input, args...);
				auto mapEnd = std::chrono::steady_clock::now();
				// Check if 0 nanoseconds have passed 
				if ((mapEnd - mapStart).count() == 0) {
					nulified++;
				}
				overallTime += (mapEnd - mapStart);
			}

			auto meanTime = overallTime.count() / (testCount - nulified);
			std::cout << meanTime << "\n";
			std::cout << "------------------------" << '\n';

			threadCount *= 2;
	 	}
		blockCount *= 2;
	}
}

int elemental(int a, int b) {
	return a + b;
}

// MAIN
int main()
{
	int nItems = 5000;
	std::vector<int> in(nItems);
	std::vector<int> out(nItems);
	// input data
	for (int i = 0; i < nItems; i++) {
		in[i] = i;
	}
	// elemental argument
	int elemental_arg = 2;

	// call test map
	mapTester(elemental, in, out, elemental_arg);


	// Check output
	//for (int i = 0; i < nItems; i++)
	//	std::cout << "Item [" << i << "] = " << out[i] << std::endl;

	//normal_main();

	std::cin.get();
}
