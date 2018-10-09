#pragma
#ifndef HPP_GRAPHTEST
#define HPP_GRAPHTEST

#include <iostream>
#include <fstream>
#include "TesterMap.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////

struct Grapher_Data {
    int first;
    int second;
    long double value;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
enum class Grapher_Type : unsigned char {
    PLOT_1 = 0x01,                  // Will plot Type 1 plot       data = { Threads, blocks, val}
    PLOT_2 = 0x02,                  // Will plot Type 2 plot       data = { Threads, blocks, val}
    PLOT_3 = 0x04,                  // Will plot Type 3 plot       data = { Threads, blocks, val}
    PLOT_INVERT = 0x08              // Will plot inverted args     data = { blocks, threads, val}
};
inline Grapher_Type operator | (Grapher_Type lhs, Grapher_Type rhs)
{
	return (Grapher_Type)(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs));
}

inline Grapher_Type operator & (Grapher_Type lhs, Grapher_Type rhs)
{
	return (Grapher_Type)(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs));
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

class Grapher {
public:
    Grapher(){}
    //Grapher(testArgument tArg, testArgument bArg, Grapher_Type gt) : gt(gt),tArg(tArg),bArg(bArg){ }

    Grapher_Data *data = nullptr;

private:
    Grapher_Type gt;
    testArgument tArg;
    testArgument bArg;
    void printData();
    void printPlotter();
};


#endif /* end of include guard: HPP_GRAPHTEST */
