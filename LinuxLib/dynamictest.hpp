#ifndef _DYNAMICTEST_HPP
#define _DYNAMICTEST_HPP


size_t dnItems = 100000;
int delemental(int a, int b){
    return a+b;
}


void dynamictest(){

    // declare
    std::vector<int> in(dnItems);
	std::vector<int> out(dnItems);

    // fill
    int elemental_arg = 2;
    for (size_t i = 0; i < dnItems; i++) {
		in[i] = i;
	}

    // calculate
    auto dynamicMap = DynamicMap(delemental,10, dnItems/10);
    dynamicMap(out, in, elemental_arg);


    // print
    for (size_t i = 0; i < dnItems; i+= 500) {
		if(in[i] + elemental_arg != out[i])
            std::cout<< i<<std::endl;
	}


}









#endif
