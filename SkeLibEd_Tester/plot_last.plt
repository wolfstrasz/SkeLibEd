set multiplot
set terminal pngcairo enhanced font "arial, 10" fontscale 1.0 size 600, 400
set xlabel "threads" 
set ylabel "block count" 
set xtics border 2, 1, 32
set ytics border 2, 1, 32
set view 10,75
set dgrid3d
set hidden3d
set title "Problem: colatz 271\nTests per AVG: 100\nInput size: 100000\nCPUs: 8\nSeq. Time:0.113570"
set output "colatz 271_100000_T-2-32_B-2-32_8.png"
set zlabel "exec. time"
splot "colatz 271_100000_T-2-32_B-2-32_8_time.dat" using 1:2:3 title "execution time" with lines
set zlabel "speedup"
splot "colatz 271_100000_T-2-32_B-2-32_8_speedup.dat" using 1:2:3 title "speedup" with lines
