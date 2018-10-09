#!/usr/bin/gnuplot -persist

set terminal pngcairo enhanced font "arial,10" fontscale 1.0 size 600, 400 
set output 'data.png'

set title "Problem: a+b \n Input size: 10^6"
set zlabel  "exec \n time"
set xlabel "number of threads"
set ylabel "block count"

//set xtics offset 0,0.5 border -5,1,5
#set label "yield point" at 0.03,2
#set xrange [1:10]
#set yrange [0.0:0.02]
#set grid
#plot for [i=1:3] "data.dat" u (column(0)):i:xtic(i) w l title "blockSize=".i

#plot "data.dat" u 2:1 title 'ThreadCountA' with lines, "data.dat" u 3:1 title "bb" with lines

#plot "data.dat" using 1:2 title "Sequential" with lines, \
#	for[i=3:5] "data.dat" using 1:i title 'Par.Blocksize'.(i-2) with lines

#      set multiplot;                          # get into multiplot mode
#      set size 1,0.5;  
#     set origin 0.0,0.5;   plot sin(x); 
#      set origin 0.0,0.0;   plot cos(x)
#      unset multiplot                         # exit multiplot mode

set dgrid3d
splot "data.dat" using 1:2:3 with lines
