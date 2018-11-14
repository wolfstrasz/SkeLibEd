#pragma once
#ifndef _TEST_MANDELBROT_HPP
#define _TEST_MANDELBROT_HPP

#include "Map.hpp"
#include "DynamicMap.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>



namespace mandelbrot {
#define mandelbrot_testcount 10

	struct pixel_t{
		int r;
		int g;
		int b;

		// Operator overtide for !=
		bool operator !=(const pixel_t& a) const
		{
			return (r != a.r || g != a.g || b != a.b);
		}
	};

	pixel_t mandelbrot_elemental(size_t taskid, double magnification, size_t xres, size_t yres, size_t itermax) {

		pixel_t pixel;
		double x, xx, y, cx, cy;
		int iteration, hx, hy, i;
		double magnify = magnification;
		hx = taskid % xres;
		hy = taskid / yres;
		cx = (((float)hx) / ((float)xres) - 0.5) / magnify * 3.0 - 0.7;
		cy = (((float)hy) / ((float)yres) - 0.5) / magnify * 3.0;
		x = 0.0; y = 0.0;
		for (iteration = 1; iteration < itermax; iteration++) {
			xx = x * x - y * y + cx;
			y = 2.0*x*y + cy;
			x = xx;
			if (x*x + y * y > 100.0)  iteration = itermax + 1;
		}

		if (iteration <= itermax) {
			pixel.r = 0; pixel.g = 255; pixel.b = 255;
		}
		else {
			pixel.r = 180; pixel.g = 0; pixel.b = 0;
		}
		return pixel;
	}

	void test(size_t threadcount, size_t blockcount, size_t ixc, size_t iyc,size_t itermax, double arg) {
		int itemcount = ixc * iyc;
		// output file
		std::string outfileName = "mandel_" + std::to_string(threadcount) + "T_"
			+ std::to_string(blockcount) + "B_" + std::to_string(ixc) + "D_"
			+ std::to_string(itermax) + "IT_" + std::to_string((int)arg) + "MAG";
		std::ofstream outfile;
		outfile.open(outfileName);


		//initialisation
		std::vector <int> in(itemcount);
		std::vector <pixel_t> dynMapOut(itemcount);
		std::vector <pixel_t> mapOut(itemcount);

		for (size_t i = 0; i < itemcount; i++) {
			in[i] = i;
		}
		std::chrono::duration<double, std::milli> time;
		auto start = std::chrono::system_clock::now();
		time = start - start;

		// Static Map
		// ----------------------------------------------------------
		for (size_t test = 0; test < mandelbrot_testcount; test++) {
			auto start = std::chrono::system_clock::now();

			auto map = Map(mandelbrot_elemental, threadcount, blockcount);
			map(mapOut, in, arg, ixc, iyc, itermax);

			auto end = std::chrono::system_clock::now();
			time += (end - start);
		}
		outfile << "SMAP: " << std::to_string(time.count() / mandelbrot_testcount) << std::endl;

		// reset time
		time = start - start;

		// Dynamic map
		// ----------------------------------------------------------
		for (size_t test = 0; test < mandelbrot_testcount; test++) {
			auto start = std::chrono::system_clock::now();

			auto dynamicMap = DynamicMap(mandelbrot_elemental, threadcount, itemcount / (blockcount * threadcount));
			dynamicMap(dynMapOut, in, arg, ixc, ixc, itermax);

			auto end = std::chrono::system_clock::now();
			time += (end - start);
		}
		outfile << "DMAP: " << std::to_string(time.count() / mandelbrot_testcount) << std::endl;

		// close file
		outfile.close();

		// Check if output is same
		// ----------------------------------------------------------
		bool same = true;
		for (size_t i = 0; i < itemcount; i++) {
			if (dynMapOut[i] != mapOut[i]) {
				same = false;
				break;
			}
		}
		if (!same) {
			for (size_t i = 0; i < itemcount; i += 10000) {
				if (dynMapOut[i] != mapOut[i])std::cout << i << std::endl;
			}
		}
	}
}
#endif
