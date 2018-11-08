#pragma once
#ifndef TESTMAPS_HPP
#define TESTMAPS_HPP

#include "Map.hpp"
#include "DynamicMap.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>


#define tm_ITERMAX 1000
#define tm_testCount 10

// COLATZ
// ---------------------------------------------
void tm_justAMoment(size_t n = 871) {

	if (n < 1) return;

	while (n != 1) {

		if (n % 2) n = 3 * n + 1;
		else n = n / 2;// for (size_t i = 0; i < tm_Items; i++) {
		// 	colatzSeqOut[i] = colatz(colatzIn[i], sumArg);
		// }
	}
}

int colatz(int a, int b) {
	tm_justAMoment();
	return a + b;
}

// MANDELBROT
// -----------------------------------------------
typedef struct {
	int r;
	int g;
	int b;
} tm_pixel_t;

tm_pixel_t mandel(int taskid, double magnification, size_t tm_XRES, size_t tm_YRES) {

	tm_pixel_t pixel;
	double x, xx, y, cx, cy;
	int iteration, hx, hy, i;
	//double magnify = 1.0;             /* no magnification */
	double magnify = magnification;
	hx = taskid % tm_XRES;
	hy = taskid / tm_YRES;
	cx = (((float)hx) / ((float)tm_XRES) - 0.5) / magnify * 3.0 - 0.7;
	cy = (((float)hy) / ((float)tm_YRES) - 0.5) / magnify * 3.0;
	x = 0.0; y = 0.0;
	for (iteration = 1; iteration<tm_ITERMAX; iteration++) {
		xx = x * x - y * y + cx;
		y = 2.0*x*y + cy;
		x = xx;
		if (x*x + y * y>100.0)  iteration = tm_ITERMAX + 1;
	}

	if (iteration <= tm_ITERMAX) {
		pixel.r = 0; pixel.g = 255; pixel.b = 255;
	}
	else {
		pixel.r = 180; pixel.g = 0; pixel.b = 0;
	}
	return pixel;
}


// APPLICATION
// --------------------------------------------
void TestMaps(int flag, size_t threadcount, size_t blockcount, size_t icx, size_t icy, double arg) {

	auto start = std::chrono::system_clock::now();
	size_t itemcount = icx * icy;

	if (flag == 1) {
		// TEST COLATZ
		// ----------------------------------------------
		// output file
		std::string outfileName = "colatz871" + std::to_string(threadcount) + "T_"
			+ std::to_string(blockcount) + "B_" + std::to_string(itemcount) + "I";
		std::ofstream outfile;
		outfile.open(outfileName);

		// initialisation
		std::vector<int> colatzIn(itemcount);
		std::vector<int> colatzSeqOut(itemcount);
		std::vector<int> colatzDynMapOut(itemcount);
		std::vector<int> colatzMapOut(itemcount);

		int sumArg = arg;
		for (size_t i = 0; i < itemcount; i++) {
			colatzIn[i] = i;
		}

		// Sequantial
		auto start = std::chrono::system_clock::now();
		for (size_t i = 0; i < itemcount; i++) {
			colatzSeqOut[i] = colatz(colatzIn[i], sumArg);
		}
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> resultTime = end - start;

		outfile << "SEQ: " << std::to_string (resultTime.count())<<std::endl;


		//Dynamic Map
		resultTime = end - end;
		for (size_t test = 0; test < tm_testCount ; test ++){
			auto start = std::chrono::system_clock::now();

			auto dynamicMap = DynamicMap(colatz, threadcount, itemcount / (blockcount * threadcount));
			dynamicMap(colatzDynMapOut, colatzIn, sumArg);

			auto end = std::chrono::system_clock::now();
			resultTime += (end - start);
		}
		outfile << "DMAP: " << std::to_string (resultTime.count() / tm_testCount)<<std::endl;

		// Static Map
		resultTime = end - end;
		for (size_t test = 0; test < tm_testCount ; test ++){
			auto start = std::chrono::system_clock::now();

			auto map = Map(colatz, threadcount, blockcount);
			map(colatzMapOut, colatzIn, sumArg);

			auto end = std::chrono::system_clock::now();
			resultTime += (end - start);
		}
		outfile << "SMAP: " << std::to_string (resultTime.count() / tm_testCount)<<std::endl;

		for (size_t i = 0 ; i< itemcount; i+=10000){
			if (colatzDynMapOut[i] != colatzSeqOut[i])std::cout<<i<<std::endl;
		}
		outfile.close();
	}

	 if (flag == 2) {
	
	 	// MANDELBROT
	 	// -----------------------------------------------
		 std::string outfileName = "mandel" + std::to_string((int)arg) + "_" + std::to_string(threadcount) + "T_"
			 + std::to_string(blockcount) + "B_" + std::to_string(itemcount) + "I";
		 std::ofstream outfile;
		 outfile.open(outfileName);


		 //initialisation
	 	std::vector <int> manIn(itemcount);
	 	std::vector <tm_pixel_t> manDynMapOut(itemcount);
	 	std::vector <tm_pixel_t> manMapOut(itemcount);
	 	std::vector <tm_pixel_t> manSeqOut(itemcount);
	 	for (size_t i = 0; i < itemcount; i++) {
	 		manIn[i] = i;
	 	}
	
	 	// Sequantial
		auto start = std::chrono::system_clock::now();
	 	for (size_t i = 0; i < itemcount; i++) {
	 		manSeqOut[i] = mandel(manIn[i], arg);
	 	}
		auto end = std::chrono::system_clock::now();
		std::chrono::duration<double, std::milli> resultTime = end - start;

		outfile << "SEQ: " << std::to_string(resultTime.count()) << std::endl;


	 	// Dynamic map
		resultTime = end - end;
		for (size_t test = 0; test < tm_testCount; test++) {
			auto start = std::chrono::system_clock::now();

			auto dynamicMap = DynamicMap(mandel, threadcount, itemcount / (blockcount * threadcount));
			dynamicMap(manDynMapOut, manIn, arg, icx, icx);

			auto end = std::chrono::system_clock::now();
			resultTime += (end - start);
		}

	 	// Static Map
		outfile << "DMAP: " << std::to_string(resultTime.count() / tm_testCount) << std::endl;
		for (size_t test = 0; test < tm_testCount; test++) {
			auto start = std::chrono::system_clock::now();

			auto map = Map(mandel, threadcount, blockcount);
			map(manDynMapOut, manIn, arg, icx, icy);

			auto end = std::chrono::system_clock::now();
			resultTime += (end - start);
		}
		outfile << "SMAP: " << std::to_string(resultTime.count() / tm_testCount) << std::endl;

	 }
}







#endif
