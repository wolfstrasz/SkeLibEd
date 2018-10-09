// ------------------------------------------------------
// MAP TESTER:
// func 	- elemental function used in the map pattern
// input 	- input vector of items
// args		- arguments list
// ------------------------------------------------------
/*
std::cout << "Map timing (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
std::cout << "Map timing (�s): " << std::chrono::duration_cast<std::chrono::microseconds>(mapEnd - mapStart - chronoCallTime).count() << std::endl;
std::cout << "Map timing (ns): " << std::chrono::duration_cast<std::chrono::nanoseconds> (mapEnd - mapStart - chronoCallTime).count() << std::endl;
*/
#pragma once
#ifndef _HPP_TESTER_MAP
#define _HPP_TESTER_MAP
#include <iostream>
#include <vector>
#include <chrono>
#include "Map.hpp"
#include "GraphTest.hpp"

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

		long double seqTiming, parTiming;
		// Test sequential time
		auto seqTime = runSeqTest(func, input, output, args...);
		std::cout << seqTime.count() << "\n";
		// Test parallel time
		// ------------------
		auto start = std::chrono::system_clock::now();
		auto overallTime = std::chrono::duration<double>(start - start);

		// Run for each block size
		for (bi = bArg.start; bi <= bArg.end;) {

			// Run for each thread count
			for (ti = tArg.start; ti <= tArg.end;) {



				nullifiedTestCount = 0;
				overallTime -= overallTime;
				//std::cout<< overallTime.count()<< "\n";
				// Run for number of tests per arguments
				for (int t = 0; t < testCount; t++) {
					overallTime += runParTest(func, input, output, args...);
					//std::cout<< overallTime.count()<< "\n";
				}

				auto meanTime = overallTime.count() / (testCount - nullifiedTestCount);
				if ( (seqTime.count() / meanTime) > 3.6f) {
					std::cout << "---------------------------------------------" << '\n';
					std::cout << "blockCount:      " << bi << '\n';
					std::cout << "threadCount:     " << ti << '\n';
					std::cout << "meanTime:        " << meanTime << "\n";
					std::cout << "advance:         " << seqTime.count() / meanTime << "\n";
					std::cout << "nullified tests: " << nullifiedTestCount << "\n";
				}
				// Increase threads
				ti += tArg.inc;
				ti *= tArg.mul;
			}

			// Increase blocks
			bi += bArg.inc;
			bi *= bArg.mul;
		}
	}

	void setGrapher(Grapher &grapher){
		this-> grapher = grapher;
	}

private:
	// Grapher
	Grapher grapher;

	// Keep track of tests and nullified tests
	int testCount;
	int nullifiedTestCount;

	// Keeps tracks of blocks count arguments
	testArgument bArg;
	int bi;

	// Keeps tracks of thread count arguments
	testArgument tArg;
	int ti;

	// Test Parallel time
	template<typename EL, typename IN, typename OUT, typename ...ARGs >
	std::chrono::duration< long double> runParTest(EL &func, std::vector<IN> &input, std::vector<OUT> &output, ARGs... args) {
		// Map skeleton
		auto map = Map(func, ti, bi);

		// Execute Map skeleton
		auto start = std::chrono::system_clock::now();
		map(output, input, args...);
		auto end = std::chrono::system_clock::now();

		// Check if 0 nanoseconds have passed
		if ((end - start).count() == 0.0f) {
			nullifiedTestCount++;
		}
		std::chrono::duration<long double> diff = end-start;
		return diff;
	}

	// Test Sequential time
	template<typename EL, typename IN, typename OUT, typename ...ARGs >
	std::chrono::duration<long double> runSeqTest(EL &func, std::vector<IN> &input, std::vector<OUT> &output, ARGs... args) {

		size_t i;
		auto start = std::chrono::system_clock::now();
		for (i = 0; i < output.size(); i++) {
			//std::cout << "input[" << i << "]" << " = " << input[i];
			output[i] = func(input[i], args...);
			//std::cout << " => output[" << i << "]"<< " = " << output[i]<< "\n";
		}
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<long double> diff = end-start;
		return diff;
	}
};
#endif // !_HPP_TESTER_MAP
