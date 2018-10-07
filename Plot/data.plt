#!/usr/bin/gnuplot -persist

set title "experiment1"
set xlabel "number of threads"
set ylabel "exec time"
set grid
plot "data.dat" u (column(0)):2:xtic(1) w l title "","data.dat" u (column(0)):3:xtic(1) w l title ""