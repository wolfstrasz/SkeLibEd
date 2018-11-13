#include <iostream>
#include <limits.h>
#include <chrono>
#include <fstream>

#include "TestCollatz.hpp"
#include "TestMandelbrot.hpp"
#include "TestNBody.hpp"

int main(char* argv[]){

	size_t func = 0;
	size_t thrc = 0;
	size_t blkc = 0;
	size_t ic = 0;
	size_t iters = 0;
	double arg = 0.0f;
	//--------------------------------------------
	int index = 1;
	func  = strtol(argv[index++], nullptr, 0);
	thrc  = strtol(argv[index++], nullptr, 0);
	blkc  = strtol(argv[index++], nullptr, 0);
	ic    = strtol(argv[index++], nullptr, 0);
	iters = strtol(argv[index++], nullptr, 0);
	arg   = strtod(argv[index++], nullptr);

	std::cout << "TEST:    " << func << std::endl;		// test functionality
	std::cout << "THREADS: " << thrc << std::endl;		// number of threads
	std::cout << "BLOCKS:  " << blkc << std::endl;		// number of blocks
	std::cout << "IXC:     " << ic << std::endl;		// number of items in a dimension
	std::cout << "ITERS:   " << iters << std::endl;		// number of iterations
	std::cout << "ARG:     " << arg << std::endl;		// additional arg

	if (func == 1) {
		collatz::test(thrc, blkc, ic, arg);
	}
	else if (func == 2) {
		mandelbrot::test(thrc, blkc, ic, ic, iters, arg);
	}
	else if (func == 3) {
		nbody::test(thrc, blkc, ixc, iyc);
	}

return 0;
}
