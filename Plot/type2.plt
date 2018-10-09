#!/usr/bin/gnuplot -persist

# Sets output png and fonts
set terminal pngcairo enhanced font "arial,10" fontscale 1.0 size 600, 400 
set output 'type2.png'

# Set title
set title "Problem: a+b \n Input size: 10^6"

# Axis labels
set zlabel  "exec \n time"
set xlabel "number of threads"
set ylabel "block count"

set xtics border 0,1,5			# X tics
set ytics border 0,4,16			# Y tics

# sets coloured graph scene
set pm3d

set view 10,75				# Set angle of view
set dgrid3d 4,4				# set a 3D grid with 4by4 vertices

# Hides data that can't be seen from view point
set hidden3d				

# PLOT THE GRAPH
splot "type2.dat" using 1:2:3 title "something" with lines
