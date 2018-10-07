// ------------------------------------------------------
// MAP TESTER:
// func 	- elemental function used in the map pattern
// input 	- input vector of items
// args		- arguments list
// ------------------------------------------------------
/*
std::cout << "Map timing (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
std::cout << "Map timing (µs): " << std::chrono::duration_cast<std::chrono::microseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
std::cout << "Map timing (ns): " << std::chrono::duration_cast<std::chrono::nanoseconds> (mapEnd - mapStart - chronoCallTime).count() << std::endl;
*/
#pragma once
#ifndef _HPP_TESTER_MAP
#define _HPP_TESTER_MAP
#include <iostream>
#include <vector>
#include <chrono>
#include "Map.hpp"

#define SCNS std::chrono::nanoseconds

struct testArgument {
	int start;
	int end;
	int inc;
	int mul;
};

class TesterMap {
public:
	TesterMap(int testCount, testArgument tArg, testArgument bArg) 
		: testCount(testCount), tArg(tArg), bArg(bArg) { }
	TesterMap() { }

	template<typename EL, typename IN, typename OUT, typename ...ARGs >
	void test(EL &func, std::vector<IN> &input, std::vector<OUT> &output, ARGs... args) {
		
		// Test sequential time
		auto seqTime = runSeqTest(func, input, output, args...);
		std::cout << seqTime.count() << "\n";
		// Test parallel time
		// ------------------S
		auto start = std::chrono::steady_clock::now();
		auto overallTime = start - start;

		// Run for each block size
		for (bi = bArg.start; bi <= bArg.end;) {

			// Run for each thread count
			for (ti = tArg.start; ti <= tArg.end;) {


				std::cout << "---------------------------------------------" << '\n';
				std::cout << "blockCount:      " << bi << '\n';
				std::cout << "threadCount:     " << ti << '\n';

				nullifiedTestCount = 0;
				overallTime -= overallTime;
				// Run for number of tests per arguments
				for (int t = 0; t < testCount; t++) {
					overallTime += runParTest(func, input, output, args...);
				}

				auto meanTime = overallTime.count() / (testCount - nullifiedTestCount);
				std::cout << "meanTime:        " << meanTime << "\n";
				std::cout << "nullified tests: " << nullifiedTestCount << "\n";

				// Increase threads
				ti += tArg.inc;
				ti *= tArg.mul;
			}

			// Increase blocks
			bi += bArg.inc;
			bi *= bArg.mul;
		}
	}


private:
	// Keep track of tests and nullified tests
	int testCount;
	int nullifiedTestCount;
	// Keeps tracks of blocks count arguments
	testArgument bArg;		
	int bi;
	// Keeps tracks of thread count arguments
	testArgument tArg;		// Thread test argument
	int ti;

	// Test Parallel time
	template<typename EL, typename IN, typename OUT, typename ...ARGs >
	SCNS runParTest(EL &func, std::vector<IN> &input, std::vector<OUT> &output, ARGs... args) {
		// Map skeleton
		auto map = Map(func, ti, bi);

		// Execute Map skeleton
		auto mapStart = std::chrono::steady_clock::now();
		map(output, input, args...);
		auto mapEnd = std::chrono::steady_clock::now();

		// Check if 0 nanoseconds have passed 
		if ((mapEnd - mapStart).count() == 0) {
			nullifiedTestCount++;
		}
		return mapEnd - mapStart;
	}

	// Test Sequential time
	template<typename EL, typename IN, typename OUT, typename ...ARGs >
	SCNS runSeqTest(EL &func, std::vector<IN> &input, std::vector<OUT> &output, ARGs... args) {

		size_t i;
		auto seqStart = std::chrono::steady_clock::now();
		for (i = 0; i < output.size(); i++) {
			output[i] = func(input[i], args...);
		}
		auto seqEnd = std::chrono::steady_clock::now();

		return seqEnd - seqStart;
	}
};
#endif // !_HPP_TESTER_MAP
