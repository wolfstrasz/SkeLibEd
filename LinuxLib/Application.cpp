#include <iostream>
#include <limits.h>
#include "skelibed.hpp"

#define NITEMS 5127
int elemental(int a, int b) {
	return a + b;
}

int combiner (int a, int b) {
	return (a+b) % INT_MAX;
}

int main()
{
	// ---------------------------------------------
	// output data
	std::vector<int> out(NITEMS);
	int sum;
	// input data
	std::vector<int> in(NITEMS);
	for (size_t i = 0; i < NITEMS; ++i) {
		in[i] = i;
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
	//std::cin.get();
}
/*dsadasd*/
