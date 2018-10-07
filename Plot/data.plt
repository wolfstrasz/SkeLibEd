#!/usr/bin/gnuplot -persist

set terminal pngcairo enhanced font "arial,10" fontscale 1.0 size 600, 400 
set output 'out.png'

set title "experiment1"
set xlabel "number of threads"
set ylabel "exec time"
set grid
plot for [i=1:3] "data.dat" u (column(0)):i:xtic(i) w l title "blockSize=".i