#ifndef _MANDELBROT_HPP
#define _MANDELBROT_HPP


//#include "DynamicMap.hpp"
#include "DynamicMap2.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include "TestMandelbrot.hpp"



void draw(size_t threadcount, size_t blockcount, size_t ixc, size_t iyc,size_t itermax, double arg){
    FILE *outfile;
    int itemcount = ixc * iyc;
    //std::ofstream outfile;
    std::string outfileName = "mandel_" + std::to_string(threadcount) + "T_"
        + std::to_string(blockcount) + "B_" + std::to_string(ixc) + "D_"
        + std::to_string(itermax) + "IT_" + std::to_string((int)arg) + "MAG.ppm";

    std::vector <int> in(itemcount);
    std::vector <mandelbrot::pixel_t> dynMapOut(itemcount);

    for (size_t i = 0; i < itemcount; i++) {
        in[i] = i;
    }
    
    auto dynamicMap = DynamicMap(mandelbrot::mandelbrot_elemental, threadcount, itemcount / (blockcount * threadcount));
    dynamicMap(dynMapOut, in, arg, ixc, iyc, itermax, itemcount / (blockcount * threadcount));

    outfile = fopen(outfileName.c_str(), "w");
    //std::ofstream f(outfileName, std::ios::out | std::ios::binary);
    fprintf(outfile,"P6\n# CREATOR: Eric R Weeks / mandel program\n");
    fprintf(outfile,"%d %d\n255\n",ixc,iyc);
    for (size_t i = 0; i<ixc*iyc; i++) {
      fputc((char)dynMapOut[i].r,outfile);
      fputc((char)dynMapOut[i].g,outfile);
      fputc((char)dynMapOut[i].b,outfile);
    }
    fclose(outfile);

}

#endif
