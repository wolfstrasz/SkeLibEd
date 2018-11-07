#pragma once
#ifndef TESTMAPS_HPP
#define TESTMAPS_HPP

#include "Map.hpp"
#include "DynamicMap.hpp"
#include <chrono>
#include <iostream>

#define tm_XRES 2048
#define tm_YRES 2048
#define tm_ITERMAX 1000
#define tm_Items 1000000
#define tm_threads 4
#define tm_blocks 10
#define tm_magnify 1.0f
// COLATZ
void tm_justAMoment(size_t n = 871) {

	if (n < 1) return;

	while (n != 1) {

		if (n % 2) n = 3 * n + 1;
		else n = n / 2;
	}
}

int colatz(int a, int b) {
	tm_justAMoment();
	return a + b;
}

// MANDELBROT
typedef struct {
	int r;
	int g;
	int b;
} tm_pixel_t;

tm_pixel_t mandel(int taskid, double magnification) {

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


void TestMaps(int flag) {

	if (flag == 1) {
		// TEST COLATZ
		// ----------------------------------------------
		std::vector<int> colatzIn(tm_Items);
		std::vector<int> colatzSeqOut(tm_Items);
		std::vector<int> colatzDynMapOut(tm_Items);
		std::vector<int> colatzMapOut(tm_Items);
		int sumArg = 123;
		for (size_t i = 0; i < tm_Items; i++) {
			colatzIn[i] = i;
		}

		// Sequantial
		for (size_t i = 0; i < tm_Items; i++) {
			colatzSeqOut[i] = colatz(colatzIn[i], sumArg);
		}

		// Dynamic Map
		auto dynamicMap = DynamicMap(colatz, tm_threads, tm_Items / tm_blocks);
		dynamicMap(colatzDynMapOut, colatzIn, sumArg);

		// Static Map
		auto map = Map(colatz, tm_threads, tm_blocks);
		map(colatzMapOut, colatzIn, sumArg);

	}
	
	if (flag == 2) {

		// MANDELBROT
		// -----------------------------------------------

		size_t nItems = tm_XRES * tm_YRES;
		std::vector <int> manIn(nItems);
		std::vector <tm_pixel_t> manDynMapOut(nItems);
		std::vector <tm_pixel_t> manMapOut(nItems);
		std::vector <tm_pixel_t> manSeqOut(nItems);
		for (int i = 0; i < nItems; i++) {
			manIn[i] = i;
		}

		// Sequantial
		for (int i = 0; i < nItems; i++) {
			manSeqOut[i] = mandel(manIn[i], tm_magnify);
		}

		// Dynamic map
		auto dynamicMap = DynamicMap(mandel, tm_threads, tm_Items / tm_blocks);
		dynamicMap(manDynMapOut, manIn, tm_magnify);

		// Static Map
		auto map = Map(mandel, tm_threads, tm_blocks);
		map(manDynMapOut, manIn, tm_magnify);

	}
}







#endif