#include <iostream>
#include <limits.h>
#include "skelibed.hpp"
#include <chrono>
//#include "normal_test.hpp"

// MAIN
int main()
{
	//normal_main()

	// Get time of chrono call
	// -----------------------
	std::chrono::steady_clock::time_point chronoStart = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point chronoEnd = std::chrono::steady_clock::now();
	auto chronoCallTime = chronoEnd - chronoStart;
	std::cout << "chrono call = " << std::chrono::duration_cast<std::chrono::microseconds>(chronoCallTime).count() << std::endl;
	std::cout << "chrono call = " << std::chrono::duration_cast<std::chrono::nanoseconds> (chronoCallTime).count() << std::endl;

	// Calculate time of task
	// ----------------------
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	for (int i = 1; i < 1000000; i++) {
		int a;
	}

	// Check time without precision
	// ----------------------------
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;
	std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;
	
	// Remove chrono call time for better precision;
	// ---------------------------------------------
	end -= chronoCallTime;

	std::cout << "Time difference precision = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << std::endl;
	std::cout << "Time difference precision = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << std::endl;

	std::cin.get();
}
