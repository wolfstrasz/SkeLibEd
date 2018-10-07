#include <iostream>
#include <limits.h>
#include <chrono>
#include "skelibed.hpp"
#include "TesterMap.hpp"
//#include "normal_test.hpp"

void justAMoment(size_t n = 871) {

    if( n < 1 ) return;

    while(n!=1) {

        if(n%2) n = 3*n + 1;
        else n = n / 2;
    }
}

int elemental(int a, int b) {
	justAMoment();
	return a + b;
}


// MAIN
int main()
{
	size_t nItems = 100000;
	std::vector<int> in(nItems);
	std::vector<int> out(nItems);
	// input data
	for (size_t i = 0; i < nItems; i++) {
		in[i] = i;
	}
	// elemental argument
	int elemental_arg = 2;

	// Thread test argument and Block test argument
	testArgument ti = { 2, 128, 0, 2 };
	testArgument bi = { 2, 128, 0, 2 };
	int tests = 10;
	TesterMap tmap(tests, ti, bi);
	tmap.test(elemental, in, out, elemental_arg);


	// Check output
	//for (int i = 0; i < nItems; i++)
		std::cout << "Item [" << nItems - 1 << "] = " << out[nItems-1] << std::endl;

	//normal_main();

	//std::cin.get();
}
