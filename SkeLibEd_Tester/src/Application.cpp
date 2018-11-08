#include <iostream>
#include <limits.h>
#include <chrono>
#include <fstream>
//#include "TesterMap.hpp"
//#include "skelibed.hpp"

#include "TestMaps.hpp"


int main(char* argv[]){
	int func = strtol(argv[1], nullptr, 0);
	int thrc = strtol(argv[2], nullptr, 0);
	int blkc = strtol(argv[3], nullptr, 0);
	int ixc = strtol(argv[4], nullptr, 0);
	int iyc = strtol(argv[5], nullptr, 0);
	double arg = strtod(argv[6], nullptr);


   // TestMaps(func, thrc, blkc, ixc,iyc,arg);

return 0;
}
//
// void justAMoment(size_t n = 871) {
//
// 	if (n < 1) return;
//
// 	while (n != 1) {
//
// 		if (n % 2) n = 3 * n + 1;
// 		else n = n / 2;
// 	}
// }
//
// int elemental(int a, int b) {
// 	justAMoment();
// 	return a + b;
// }
//
// /////////////////////////////////////////////
//
// #define XRES 2048
// #define YRES 2048
// #define ITERMAX 1000
// FILE *outfile;
//
// typedef struct {
// 	int r;
// 	int g;
// 	int b;
// } pixel_t;
//
// // FILE *outfile;
//
// pixel_t mandel(int taskid) {
//
// 	pixel_t pixel;
// 	double x, xx, y, cx, cy;
// 	int iteration, hx, hy, i;
// 	double magnify = 1.0;             /* no magnification */
//
// 	hx = taskid % XRES;
// 	hy = taskid / YRES;
// 	cx = (((float)hx) / ((float)XRES) - 0.5) / magnify * 3.0 - 0.7;
// 	cy = (((float)hy) / ((float)YRES) - 0.5) / magnify * 3.0;
// 	x = 0.0; y = 0.0;
// 	for (iteration = 1; iteration<ITERMAX; iteration++) {
// 		xx = x * x - y * y + cx;
// 		y = 2.0*x*y + cy;
// 		x = xx;
// 		if (x*x + y * y>100.0)  iteration = ITERMAX + 1;
// 	}
//
// 	if (iteration <= ITERMAX) {
// 		pixel.r = 0; pixel.g = 255; pixel.b = 255;
// 	}
// 	else {
// 		pixel.r = 180; pixel.g = 0; pixel.b = 0;
// 	}
// 	return pixel;
// }
//
//
// // MAIN
// int main()
// {
// 	size_t nItems = 100000;
// 	std::vector<int> in(nItems);
// 	std::vector<int> out(nItems);
// 	// input data
// 	for (size_t i = 0; i < nItems; i++) {
// 		in[i] = i;
// 	}
// 	// elemental argument
// 	int elemental_arg = 2;
//
// 	// Thread test argument and Block test argument
// 	testArgument ti = { 2, 32, 0, 2 };
// 	testArgument bi = { 2, 32, 0, 2 };
// 	int tests = 100;
//
// 	  // Grapher declaration
// 	TesterMap tmap("colatz 871", tests, ti, bi);
// 	tmap.test(elemental, in, out, elemental_arg);
//
// 	// size_t nItems = XRES * YRES;
// 	// std::vector <int> in(nItems);
// 	// for (int i = 0; i <XRES*YRES; i++) {
// 	// 	in[i] = i;
// 	// }
// 	// std::vector<pixel_t> image(in.size());
//
// 	// // Output results
// 	// outfile = fopen("custompicture.ppm", "w");
// 	// fprintf(outfile, "P6\n# CREATOR: Eric R Weeks / mandel program\n");
// 	// fprintf(outfile, "%d %d\n255\n", YRES, XRES);
// 	// for (size_t i = 0; i<XRES*YRES; i++) {
// 	// 	fputc((char)image[i].r, outfile);
// 	// 	fputc((char)image[i].g, outfile);
// 	// 	fputc((char)image[i].b, outfile);
// 	// }
//
// 	// // Thread test argument and Block test argument
// 	// testArgument ti = { 2, 256, 0, 2 };
// 	// testArgument bi = { 2, 1024 , 0, 2 };
// 	// int tests = 5;
//     //
// 	// // Grapher declaration
// 	// TesterMap tmap("Mandelbrot 2048 1000", tests, ti, bi);
// 	// tmap.test(mandel, in, image);
//     //
// 	// //Check output
// 	// // for (int i = 0; i < nItems; i++)
// 	// // 	std::cout << "Item [" << nItems - 1 << "] = " << out[nItems-1] << std::endl;
//     //
//
// 	std::cin.get();
// }
