#include <iostream>
#include <limits.h>
#include <chrono>
#include "skelibed.hpp"
#include "TesterMap.hpp"
//#include "normal_test.hpp"

int elemental(int a, int b) {
	return a + b;
}


// MAIN
int main()
{
	size_t nItems = 100000000;
	std::vector<int> in(nItems);
	std::vector<int> out(nItems);
	// input data
	for (size_t i = 0; i < nItems; i++) {
		in[i] = i;
	}
	// elemental argument
	int elemental_arg = 2;

	// Thread test argument and Block test argument
	testArgument ti = { 4, 4, 0, 2 };
	testArgument bi = { 10, 10, 0, 2 };
	int tests = 10;
	TesterMap tmap(tests, ti, bi);
	tmap.test(elemental, in, out, elemental_arg);


	// Check output
	//for (int i = 0; i < nItems; i++)
		std::cout << "Item [" << nItems - 1 << "] = " << out[nItems-1] << std::endl;

	//normal_main();

	//std::cin.get();
}
