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
#include <fstream>
#include <string>

struct testArgument {
	int start;
	int end;
	int inc;
	int mul;
};

class TesterMap {
public:
	TesterMap(std::string testName,int testCount, testArgument tArg, testArgument bArg)
		: testCount(testCount), tArg(tArg), bArg(bArg) {
		test_name = testName;
	}
	TesterMap() { }

	template<typename EL, typename IN, typename OUT, typename ...ARGs >
	void test(EL &func, std::vector<IN> &input, std::vector<OUT> &output, ARGs... args) {
		size = input.size();
		initDataFiles();
		
		// Test sequential time
		auto seqTime = runSeqTest(func, input, output, args...);
		seqTiming = seqTime.count();
		//std::cout << seqTiming<< "\n";

		// Test parallel time
		// ------------------
		auto start = std::chrono::system_clock::now();
		auto overallTime = std::chrono::duration<double>(start - start);

		// Run for thread count
		for (ti = tArg.start; ti <= tArg.end;) {

			// Run for each thread count
			for (bi = bArg.start; bi <= bArg.end;) {

				nullifiedTestCount = 0;
				overallTime -= overallTime;
				//std::cout<< overallTime.count()<< "\n";
				// Run for number of tests per arguments
				for (int t = 0; t < testCount; t++) {
					overallTime += runParTest(func, input, output, args...);
					//std::cout<< overallTime.count()<< "\n";
				}

				long double meanTime = overallTime.count() / (testCount - nullifiedTestCount);
				parTiming = (long double)meanTime;
				long double speedUp = seqTiming / parTiming;
				data_file << std::to_string(ti) + "\t" + std::to_string(bi) + "\t" + std::to_string(parTiming) + "\n";
				data_file2 << std::to_string(ti) + "\t" + std::to_string(bi) + "\t" + std::to_string(speedUp) + "\n";
					std::cout << "---------------------------------------------" << '\n';
					std::cout << "threadCount:     " << ti << '\n';
					std::cout << "blockCount:      " << bi << '\n';
					std::cout << "meanTime:        " << parTiming << "\n";
					std::cout << "advance:         " << seqTiming / parTiming << "\n";
					std::cout << "nullified tests: " << nullifiedTestCount << "\n";
			
				// Increase threads
				bi += bArg.inc;
				bi *= bArg.mul;
			}

			// Increase blocks
			ti += tArg.inc;
			ti *= tArg.mul;

			
		}
		initPlotter();
		closeFiles();
	}

private:
	// File to keep data
	long double seqTiming, parTiming;
	std::string test_name;
	std::string file_name;
	std::string time_file;
	std::string speedup_file;
	std::ofstream data_file;
	std::ofstream data_file2;
	std::ofstream plotter_file;
	long long size = 0;

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

	// Init data file
	void initDataFiles() {
		
		file_name = test_name + "_" 
			+ std::to_string(size) + "_"
			+ "T-" + std::to_string(tArg.start) + "-" + std::to_string(tArg.end) + "_"
			+ "B-" + std::to_string(bArg.start) + "-" + std::to_string(bArg.end) + "_"
			+ std::to_string(std::thread::hardware_concurrency());
		time_file = file_name +"_time.dat";
		speedup_file = file_name + "_speedup.dat";
		data_file.open(time_file);
		data_file2.open(speedup_file);

		//data_file.close();
		//data_file2.close();
	}

	void initPlotter() {
		data_file.close();
		data_file2.close();
		std::string plotter_name = "plot_last.plt";
		plotter_file.open(plotter_name);
		//plotter_file << "# plotting last test" + '\n';
		plotter_file << "set multiplot" << '\n';
		plotter_file << "set terminal pngcairo enhanced font \"arial, 10\" fontscale 1.0 size 600, 400" << '\n';
		plotter_file << "set xlabel \"threads\" " << '\n';
		plotter_file << "set ylabel \"block count\" " << '\n';
		plotter_file << "set xtics border " + std::to_string(tArg.start) << ", 1, " << std::to_string(tArg.end) << '\n';
		plotter_file << "set ytics border " + std::to_string(bArg.start) << ", 1, " << std::to_string(bArg.end) << '\n';
		plotter_file << "set view 10,75" << '\n';
		plotter_file << "set dgrid3d" << '\n';
		plotter_file << "set hidden3d" << '\n';
		plotter_file << "set title \"Problem: " << test_name << "\\n"
			<< "Tests per AVG: " << std::to_string(testCount) << "\\n"
			<< "Input size: " << std::to_string(size) << "\\n"
			<< "CPUs: " << std::to_string(std::thread::hardware_concurrency()) << "\\n"
			<< "Seq. Time: " << std::to_string(seqTiming) << "\"" << '\n';
		plotter_file << "set output \"" << file_name << ".png\"" << '\n';

		// Plot time
		plotter_file << "set zlabel \"exec. time\"" << '\n';
		plotter_file << "splot \"" << time_file << "\""
			<< " using 1:2:3 title \"execution time\" with lines" << '\n';

		// Plot speedup
		plotter_file << "set zlabel \"speedup\"" << '\n';
		plotter_file << "splot \"" + speedup_file << "\""
			<< " using 1:2:3 title \"speedup\" with lines" << '\n';
	}

	void closeFiles() {

		plotter_file.close();
	}
};
#endif // !_HPP_TESTER_MAP
