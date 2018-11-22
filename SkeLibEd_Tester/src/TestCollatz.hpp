#ifndef _TEST_COLLATZ_HPP
#define _TEST_COLLATZ_HPP

#include "Map.hpp"
//#include "DynamicMap.hpp"
//#include "DynamicMap2.hpp"
#include "DynamicMap3.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>

namespace collatz {
#define collatz_test_count 10

	void collatz_wait(size_t n = 871) {
		if (n < 1) return;
		while (n != 1) {
			if (n % 2) n = 3 * n + 1;
			else n = n / 2;
		}
	}

	int collatz_elemental(size_t taskid, size_t b) {
		collatz_wait(taskid);
		return taskid + b;
	}

	void test(size_t threadcount, size_t blockcount, size_t itemcount, double arg) {
		// output file
		std::string folderName = "";//"collatz1_" + std::to_string(std::thread::hardware_concurrency()) + "/";
		std::string outfileName = folderName + "collatz_" + std::to_string(threadcount) + "T_"
			+ std::to_string(blockcount) + "B_" + std::to_string(itemcount) + "I";
		std::ofstream outfile;
		outfile.open(outfileName);

		// initialisation
		std::vector<int> in(itemcount);
		std::vector<int> dynMapOut(itemcount);
		std::vector<int> mapOut(itemcount);

		for (size_t i = 0; i < itemcount; i++) {
			in[i] = i;
		}
		std::chrono::duration<double, std::milli> time;
		auto start = std::chrono::system_clock::now();
		time = start - start;


		// Static Map
		// ----------------------------------------------------------
		for (size_t test = 0; test < collatz_test_count; test++) {
			std::cout << "STATIC MAP Test: " << test << std::endl;
			auto start = std::chrono::system_clock::now();

			auto map = Map(collatz_elemental, threadcount, blockcount);
			map(mapOut, in, arg);
			auto end = std::chrono::system_clock::now();
			time += (end - start);
		}
		outfile << "SMAP: " << std::to_string(time.count() / collatz_test_count) << std::endl;

		// reset time
		time = start - start;

		// Dynamic Map
		// ----------------------------------------------------------
		for (size_t test = 0; test < collatz_test_count; test++) {
			std::cout << "DYNAMIC MAP Test: " << test << std::endl;
			auto start = std::chrono::system_clock::now();

		//	auto dynamicMap = DynamicMap(collatz_elemental, threadcount, itemcount / (blockcount * threadcount));
			auto dynamicMap = DynamicMap(collatz_elemental);
			std::cout << "DYNAMIC MAP Test: " << test << std::endl;
			dynamicMap(dynMapOut, in, arg);
			//dynamicMap.stop();

			auto end = std::chrono::system_clock::now();
			time += (end - start);
		}
		outfile << "DMAP: " << std::to_string(time.count() / collatz_test_count) << std::endl;

		// close file
		outfile.close();

		// Check if output is same
		// ----------------------------------------------------------
		bool same = true;
		for (size_t i = 0; i < itemcount; i++) {
			if (dynMapOut[i] != mapOut[i]) {
				same = false;
				break;
			}
		}
		if (!same) {
			for (size_t i = 0; i < itemcount; i += 10000) {
				if (dynMapOut[i] != mapOut[i])std::cout << i << std::endl;
			}
		}

	}

}


#endif
