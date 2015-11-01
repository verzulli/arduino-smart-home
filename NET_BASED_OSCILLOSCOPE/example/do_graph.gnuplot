set datafile separator ";"
set terminal png size 20000,800
set title "super simple oscilloscope"
set ylabel "analog-read"
set xlabel "time/sec"
set xdata time
set timefmt "%s"
set format x '%d'
set key left top
set grid
plot "raw_data_300baud_short.csv" using 1:2 with linespoints lw 2 lt 3 title 'A', \
     "raw_data_300baud_short.csv" using 1:3 with linespoints lw 2 lt 1 title 'B'
