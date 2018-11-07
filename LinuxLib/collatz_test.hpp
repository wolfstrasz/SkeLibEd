#ifndef COLLATZ_TEST_HPP
#define COLLATZ_TEST_HPP


#define clzWait 871
#define clzInputSize 1000000
#define clzArgument 400

void collatzWork(size_t n = clzWait) {
	if (n < 1) return;
	while (n != 1) {
		if (n % 2) n = 3 * n + 1;
		else n = n / 2;
	}
}

int collatz(int a) {
	collatzWork();
	return a + clzArgument;
}



// test


void collatz_test(){

    std::vector<int> inCollatz(clzInputSize);
    std::vector<int> outCollatzSeq(clzInputSize);
	std::vector<int> outCollatzDyn(clzInputSize);
    std::vector<int> outCollatzMap(clzInputSize);

	// input data
	for (size_t i = 0; i < nItems; i++) {
		in[i] = i;
	}

    


}

#endif
