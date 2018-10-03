#include <iostream>
#include "skelibed.hpp"

#define NITEMS 5000
int elemental(int a, int b) {
	return a + b;
}
int main()
{
	// input data
	std::vector<int> in(NITEMS);
	for (size_t i = 0; i < NITEMS; ++i) {
		in[i] = i;
	}
	// elemental argument
	int elemental_arg = 2;

	// output data
	std::vector<int> out(NITEMS);

	// threads
	int thread_count = 50;

	// Map skeleton
	auto map = Map(elemental, thread_count);

	// Execute Map skeleton
	//map(out, in, elemental_arg);

	// Check output 
	for (size_t i = 0; i < NITEMS; i++)
		std::cout << "Item [" << i << "] = " << out[i] << std::endl;
	std::cin.get();
}
/*dsadasd*/