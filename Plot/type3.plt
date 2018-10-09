#!/usr/bin/gnuplot -persist

set terminal pngcairo enhanced font "arial,10" fontscale 1.0 size 600, 400 
set output 'type3.png'

set title "Problem: a+b \n Input size: 10^6"
set xlabel "number of threads"
set ylabel "execution time"
set label 1 'Y-AXIS' at graph -0.2, graph 0.5

set xtics border 0,1,4
set mxtics 10
set mytics 5

set key autotitle columnhead 
set key outside


set linestyle 1 lt 16 lw 2
set key box linestyle 1

plot "type3.dat" using 1:2 with lines, \
	for[i=3:5] "type3.dat" using 1:i  with lines

