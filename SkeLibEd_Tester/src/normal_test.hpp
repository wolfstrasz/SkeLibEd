#pragma once
#ifndef _HPP_NORMAL_TEST
#define _HPP_NORMAL_TEST

#include <iostream>
#include <limits.h>
#include "skelibed.hpp"

#define NITEMS 5127
int elemental(int a, int b) {
	return a + b;
}

int combiner(int a, int b) {
	return (a + b) % INT_MAX;
}

// MAP REDUCE
auto mapper = [](int a) {
	std::list<std::pair<int, int>> res = { std::make_pair(a,1) };
	return res;
};

auto reducer = [](int key, std::list<int> values) {
	std::list<int> sum = { (int)values.size() };
	return sum;
};

auto hasher = [](int key) { return std::hash<int>{}(key); };

bool f(std::pair <int, int> a, std::pair <int, int> b) {
	return a.first < b.first;
}
// SCAN
int combine(int a, int b) {
	return a + b;
}

// NORMAL_MAIN
int normal_main()
{
	// ---------------------------------------------
	// output data
	std::vector<int> out(NITEMS);
	std::vector<std::pair<int, int>> mrout;
	std::vector<int> scout(NITEMS);
	int sum;
	// input data
	std::vector<int> in(NITEMS);
	for (size_t i = 0; i < NITEMS; i++) {
		in[i] = i;
	}
	std::vector<int> mrin(NITEMS);
	for (size_t i = 0; i <NITEMS; i += 10) {
		for (size_t j = i; j < i + 10; j++)
			mrin[j] = i;
	}
	// ---------------------------------------------

	// elemental argument
	int elemental_arg = 2;
	// threads
	int thread_count = 50;

	// ---------------------------------------------

	// Map skeleton
	auto map = Map(elemental, thread_count);
	// Execute Map skeleton
	map(out, in, elemental_arg);
	// Check output
	for (size_t i = 0; i < NITEMS; i++)
		std::cout << "Item [" << i << "] = " << out[i] << std::endl;

	// ---------------------------------------------

	// Reduce skeleton
	auto reduce = Reduce(combiner, thread_count);
	// Execute Reduce skeleton
	reduce(sum, out);
	// Check output
	std::cout << "OVERAL SUM = " << sum << std::endl;

	// -----------------------------------------------

	// MapReduce skeleton
	auto mapreduce = MapReduce(mapper, reducer, hasher, thread_count);
	// Execute MapReduce skeleton
	mapreduce(mrout, mrin);
	// Sort output
	sort(mrout.begin(), mrout.end(), f);
	// Check output
	for (size_t i = 0; i< mrout.size(); i++) {
		std::cout << "(" << mrout[i].first << ", " << mrout[i].second << ")\n";
	}
	std::cout << "SIZE OF MRIN  :" << mrin.size() << std::endl;
	std::cout << "SIZE OF MROUT :" << mrout.size() << std::endl;

	// -----------------------------------------------

	// Scan skeleton
	auto scan = Scan(combine, thread_count);
	// Execute Scan skeleton
	scan(scout, in);
	// Check output
	for (size_t i = 0; i < NITEMS; i++)
		std::cout << "Item [" << i << "] = " << scout[i] << std::endl;

	// -----------------------------------------------
	//std::cin.get();
}

#endif // !_HPP_NORMAL_TEST
