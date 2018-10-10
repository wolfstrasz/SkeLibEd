set terminal pngcairo enhanced font "arial, 10" fontscale 1.0 size 600, 400
set xlabel "threads" rotate parallel
set ylabel "execution time" rotate parallel
set logscale x
set title "Problem: Mandelbrot 2048 1000\nTests per AVG: 5\nInput size: 4194304\nCPUs: 40\nSeq. Time: 5.773179"
set output "Mandelbrot 2048 1000_time.png"
set key autotitle columnhead 
set key outside 
set linestyle 1 lt 16 lw 2 
set key box linestyle 1 
plot for[i=2:12] "Mandelbrot 2048 1000_time.dat" using 1:i:xtic(1) with lines
