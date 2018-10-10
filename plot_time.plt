set terminal pngcairo enhanced font "arial, 10" fontscale 1.0 size 600, 400
set xlabel "threads" rotate parallel
set ylabel "execution time" rotate parallel
set logscale x
set title "Problem: colatz271\nTests per AVG: 10\nInput size: 1000000\nCPUs: 40\nSeq. Time: 0.514001"
set output "colatz271_time.png"
set key autotitle columnhead 
set key outside 
set linestyle 1 lt 16 lw 2 
set key box linestyle 1 
plot for[i=2:13] "colatz271_time.dat" using 1:i:xtic(1) with lines
