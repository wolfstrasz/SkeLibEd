#!/usr/bin/gnuplot -persist

set terminal pngcairo enhanced font "arial,10" fontscale 1.0 size 600, 400 
set output 'out.png'

set title "experiment1"
set xlabel "number of threads"
set ylabel "exec time"
set grid
plot "data.dat" u (column(0)):2:xtic(1) w l title "","data.dat" u (column(0)):3:xtic(1) w l title ""